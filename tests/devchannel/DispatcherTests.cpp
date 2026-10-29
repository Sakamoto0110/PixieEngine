#include "Check.hpp"
#include "pixie/dev/Dispatcher.hpp"
#include "pixie/runtime/ProjectSession.hpp"
#include "pixie/runtime/SceneProject.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/render/gl/GlLoader.hpp"
#include "pixie_capi.h"
#include <nlohmann/json.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace {
using Json = nlohmann::json;
struct Session {
    PixieSession* value = nullptr;
    std::vector<char> output = std::vector<char>(pixie::dev::kResponseCapacity);
    int id = 0;
    Session() {
        if (pixie_session_create(96, 64, 1, &value) != PIXIE_OK) throw std::runtime_error("session creation failed");
    }
    ~Session() { pixie_session_destroy(value); }
    Json Call(const std::string& method, Json params = Json::object()) {
        const auto request = Json{{"jsonrpc", "2.0"}, {"id", ++id}, {"method", method}, {"params", params}}.dump();
        const auto result = pixie_dev_dispatch(value, request.c_str(), output.data(), static_cast<int>(output.size()));
        CHECK(result == PIXIE_OK);
        if (result != PIXIE_OK) throw std::runtime_error("dispatch ABI failure");
        return Json::parse(output.data());
    }
    Json Good(const std::string& method, Json params = Json::object()) {
        auto response = Call(method, params);
        CHECK(response.contains("result"));
        if (!response.contains("result")) throw std::runtime_error(response.dump());
        return response["result"]["value"];
    }
    std::string Create() { return Good("world.create", {{"name", "fixture"}})["id"].get<std::string>(); }
};
Json Request(const char* method, int id = 1) { return {{"jsonrpc", "2.0"}, {"id", id}, {"method", method}}; }
int rejectedDefaults = 0;
struct FaultProbe {
    int value = 1;
    FaultProbe() { if (rejectedDefaults > 0) { --rejectedDefaults; throw std::runtime_error("injected native constructor failure"); } }
};
struct ProbeProject final : pixie::Project {
    std::string_view Name() const override { return "dispatcher-fault-probes"; }
    void Describe(pixie::ComponentTypes& types) const override {
        types.Add<FaultProbe>("FaultProbe").Field<&FaultProbe::value>("value");
    }
    void Build(pixie::World& world, pixie::Camera2D&) override {
        auto entity = world.Create("fault-probe");
        entity.Add<FaultProbe>();
        pixie::AddToScene(entity);
    }
};
}

TEST(HandshakeProtocolAndNoFilesystemAliases) {
    Session s;
    CHECK(s.Call("world.stats").contains("error"));
    const auto hello = s.Good("world.hello");
    CHECK(hello["protocol_version"] == 1);
    CHECK(hello["pixie_api_version"] == PIXIE_CAPI_VERSION);
    CHECK(hello["capabilities"]["save"] == false);
    for (const char* method : {"world.save", "world.export", "world.open_project", "editor.do", "inspector.set", "world.capture"})
        CHECK(s.Call(method).contains("error"));
    CHECK(s.Call("world.stats", Json::array()).contains("error"));
    const auto invalid = pixie_dev_dispatch(s.value, "{", s.output.data(), static_cast<int>(s.output.size()));
    CHECK(invalid == PIXIE_OK);
    CHECK(Json::parse(s.output.data())["error"]["code"] == -32700);
    const auto notification = Json{{"jsonrpc", "2.0"}, {"method", "world.pause"}}.dump();
    CHECK(pixie_dev_dispatch(s.value, notification.c_str(), s.output.data(), static_cast<int>(s.output.size())) == PIXIE_OK);
    CHECK(s.output[0] == '\0');
    CHECK(s.Good("world.stats")["paused"] == true);
}

TEST(BufferPreflightAndOwnerThreadDoNotMutate) {
    Session s;
    s.Good("world.hello");
    const auto before = s.Good("world.stats")["entities"];
    const auto create = Request("world.create").dump();
    CHECK(pixie_dev_dispatch(s.value, create.c_str(), s.output.data(), 10) == PIXIE_ERROR_BUFFER);
    CHECK(s.Good("world.stats")["entities"] == before);
    PixieResult workerResult = PIXIE_OK;
    std::thread worker([&] { workerResult = pixie_dev_dispatch(s.value, create.c_str(), s.output.data(), static_cast<int>(s.output.size())); });
    worker.join();
    CHECK(workerResult == PIXIE_ERROR_ARGUMENT);
    CHECK(s.Good("world.stats")["entities"] == before);
    const std::string tooLong(pixie::dev::kMaxRequestBytes + 1, ' ');
    CHECK(pixie_dev_dispatch(s.value, tooLong.c_str(), s.output.data(), static_cast<int>(s.output.size())) == PIXIE_ERROR_ARGUMENT);
    CHECK(s.Good("world.stats")["entities"] == before);
}

TEST(SessionControlsReportActualStepsAndScale) {
    Session s;
    s.Good("world.hello");
    s.Good("world.pause");
    CHECK(s.Good("world.step", {{"count", 3}})["steps_run"] == 3);
    CHECK(s.Good("world.advance", {{"seconds", 1}})["steps_run"] == 0);
    s.Good("world.resume");
    s.Good("world.time_scale", {{"value", 8}});
    const auto advanced = s.Good("world.advance", {{"seconds", 1}})["steps_run"].get<int>();
    CHECK(advanced >= 479 && advanced <= 480);
    CHECK(s.Call("world.step", {{"count", -1}}).contains("error"));
    CHECK(s.Call("world.advance", {{"seconds", 11}}).contains("error"));
    CHECK(s.Call("world.time_scale", {{"value", 9}}).contains("error"));
    CHECK(s.Good("world.reset")["stats"]["steps"] == 0);
}

TEST(NativeSchemaNamedFieldsAndSceneIdentity) {
    Session s;
    s.Good("world.hello");
    const auto schema = s.Good("world.type", {{"name", "Transform2D"}});
    CHECK(schema["name"] == "Transform2D");
    CHECK(schema["fields"][0].contains("unit"));
    CHECK(schema["fields"][0].contains("range"));
    const auto id = s.Create();
    s.Good("world.add_component", {{"id", id}, {"type", "Transform2D"}});
    s.Good("world.set", {{"id", id}, {"field", "Transform2D.position"}, {"value", {12, 34}}});
    CHECK(s.Good("world.get", {{"id", id}, {"field", "Transform2D.position"}})["value"] == Json::array({12, 34}));
    CHECK(s.Call("world.get", {{"id", 1}, {"field", "Transform2D.position"}}).contains("error"));
    CHECK(s.Call("world.set", {{"id", id}, {"field", "Transform2D.0"}, {"value", {9, 9}}}).contains("error"));
    CHECK(s.Call("world.set", {{"id", id}, {"field", "Transform2D.position"}, {"value", {9}}}).contains("error"));
    s.Good("world.rename", {{"id", id}, {"name", "renamed"}});
    CHECK(s.Good("world.entity", {{"id", id}})["name"] == "renamed");
    s.Good("world.remove_component", {{"id", id}, {"type", "Transform2D"}});
    CHECK(s.Call("world.get", {{"id", id}, {"field", "Transform2D.position"}})["error"]["data"]["pixie_result"] == PIXIE_ERROR_NOT_FOUND);
    std::vector<char> error(4096);
    pixie_last_error(error.data(), static_cast<int>(error.size()));
    CHECK(std::string(error.data()).find("Transform2D") != std::string::npos);
    s.Good("world.destroy", {{"id", id}});
    CHECK(s.Call("world.entity", {{"id", id}}).contains("error"));
}

TEST(TextPointsAndRecordWritesUseNativeAccess) {
    Session s;
    s.Good("world.hello");
    const auto id = s.Create();
    s.Good("world.add_component", {{"id", id}, {"type", "Text"}});
    s.Good("world.set", {{"id", id}, {"field", "Text.text"}, {"value", "UTF-8: café"}});
    CHECK(s.Good("world.get", {{"id", id}, {"field", "Text.text"}})["value"] == "UTF-8: café");
    s.Good("world.add_component", {{"id", id}, {"type", "Path"}});
    s.Good("world.set", {{"id", id}, {"field", "Path.points"}, {"value", {{0, 0}, {1, 2}}}});
    CHECK(s.Good("world.get", {{"id", id}, {"field", "Path.points"}})["value"].size() == 2);
    // Native compound records have named members and positional item indices.
    s.Good("world.add_component", {{"id", id}, {"type", "CompoundCollider2D"}});
    const auto initialRecords = s.Good("world.get", {{"id", id}, {"field", "CompoundCollider2D.shapes"}})["value"].size();
    s.Good("world.record_insert", {{"id", id}, {"field", "CompoundCollider2D.shapes"}, {"item", 0}});
    s.Good("world.set", {{"id", id}, {"field", "CompoundCollider2D.shapes"}, {"index", 0}, {"member", "radius"}, {"value", 17}});
    CHECK(s.Good("world.get", {{"id", id}, {"field", "CompoundCollider2D.shapes"}, {"index", 0}, {"member", "radius"}})["value"] == 17);
    s.Good("world.record_remove", {{"id", id}, {"field", "CompoundCollider2D.shapes"}, {"item", 0}});
    CHECK(s.Good("world.get", {{"id", id}, {"field", "CompoundCollider2D.shapes"}})["value"].size() == initialRecords);
}

TEST(SnapshotReplacesExistingEntitiesAndResurrectsDestroyedOnes) {
    Session s;
    s.Good("world.hello");
    const auto id = s.Create();
    s.Good("world.add_component", {{"id", id}, {"type", "Transform2D"}});
    const auto token = s.Good("world.snapshot", {{"ids", {id}}})["token"];
    s.Good("world.set", {{"id", id}, {"field", "Transform2D.position"}, {"value", {77, 88}}});
    CHECK(s.Good("world.restore", {{"token", token}})["restored"] == 1);
    CHECK(s.Good("world.get", {{"id", id}, {"field", "Transform2D.position"}})["value"] == Json::array({0, 0}));
    s.Good("world.destroy", {{"id", id}});
    CHECK(s.Good("world.restore", {{"token", token}})["restored"] == 1);
    CHECK(s.Call("world.restore", {{"token", "unknown"}}).contains("error"));
    CHECK(s.Good("world.entity", {{"id", id}})["id"] == id);
    CHECK(s.Call("world.snapshot", {{"ids", {id, id}}}).contains("error"));
    Session other;
    other.Good("world.hello");
    CHECK(other.Call("world.restore", {{"token", token}}).contains("error"));
    s.Good("world.release_snapshot", {{"token", token}});
    CHECK(s.Call("world.restore", {{"token", token}}).contains("error"));
}

TEST(SnapshotExpiresOnWorldRebuildIncludingOutsideDispatcher) {
    pixie::ProjectSession session(std::make_unique<pixie::SceneProject>(), {});
    CHECK(session.Start({96, 64}));
    pixie::dev::WorldDispatcher dispatcher(session);
    auto call = [&](const char* method, Json params = Json::object()) {
        auto request = Request(method); request["params"] = params;
        return Json::parse(dispatcher.Dispatch(request.dump()));
    };
    call("world.hello");
    const auto id = call("world.create")["result"]["value"]["id"];
    const auto token = call("world.snapshot", {{"ids", {id}}})["result"]["value"]["token"];
    const auto revision = session.WorldRevision();
    session.Reset();
    CHECK(session.WorldRevision() != revision);
    const auto response = call("world.restore", {{"token", token}});
    CHECK(response["error"]["data"]["possibly_applied"] == false);
    CHECK(response["error"]["message"].get<std::string>().find("expired") != std::string::npos);
    session.Stop();
}

TEST(InputAndViewCommands) {
    Session s;
    s.Good("world.hello");
    s.Good("world.key", {{"key", 65}, {"down", true}});
    s.Good("world.mouse_button", {{"button", 0}, {"down", true}});
    s.Good("world.cursor", {{"x", 12}, {"y", 13}});
    s.Good("world.scroll", {{"dx", 1}, {"dy", 2}});
    CHECK(s.Call("world.key", {{"key", 9999}, {"down", true}}).contains("error"));
    const auto world = s.Good("world.view_to_world", {{"x", 12}, {"y", 13}});
    const auto view = s.Good("world.world_to_view", {{"x", world["x"]}, {"y", world["y"]}});
    CHECK(std::abs(view["x"].get<double>() - 12) < 1e-8);
    CHECK(std::abs(view["y"].get<double>() - 13) < 1e-8);
    s.Good("world.pick", {{"x", 0}, {"y", 0}});
    s.Good("world.pick_box", {{"x0", 0}, {"y0", 0}, {"x1", 96}, {"y1", 64}});
}

TEST(SnapshotReferenceClosureIsCheckedBeforeMutation) {
    Session s;
    s.Good("world.hello");
    const auto a = s.Create(), b = s.Create(), c = s.Create();
    s.Good("world.add_component", {{"id", a}, {"type", "ParticleSpawner"}});
    s.Good("world.add_component", {{"id", b}, {"type", "ParticleSpawner"}});
    s.Good("world.set", {{"id", a}, {"field", "ParticleSpawner.target"}, {"value", b}});
    s.Good("world.set", {{"id", b}, {"field", "ParticleSpawner.target"}, {"value", c}});
    const auto isolated = s.Good("world.snapshot", {{"ids", {b}}})["token"];
    const auto refused = s.Call("world.restore", {{"token", isolated}});
    CHECK(refused["error"]["data"]["possibly_applied"] == false);
    CHECK(s.Good("world.get", {{"id", a}, {"field", "ParticleSpawner.target"}})["value"] == b);
    const auto closed = s.Good("world.snapshot", {{"ids", {a, b}}})["token"];
    CHECK(s.Good("world.restore", {{"token", closed}})["restored"] == 2);
    CHECK(s.Good("world.get", {{"id", a}, {"field", "ParticleSpawner.target"}})["value"] == b);
    CHECK(s.Good("world.get", {{"id", b}, {"field", "ParticleSpawner.target"}})["value"] == c);
    const auto secondIsolated = s.Good("world.snapshot", {{"ids", {b}}})["token"];
    s.Good("world.destroy", {{"id", b}});
    const auto staleIncoming = s.Call("world.restore", {{"token", secondIsolated}});
    CHECK(staleIncoming["error"]["data"]["possibly_applied"] == false);
    CHECK(staleIncoming["error"]["message"].get<std::string>().find("incoming Entity reference") != std::string::npos);
}

TEST(EditPlayStopKeepSceneIdsAndNeverSaveThroughChannel) {
    const auto folder = std::filesystem::temp_directory_path() / std::format("pixie-devchannel-{}", std::chrono::steady_clock::now().time_since_epoch().count());
    const auto path = folder.string();
    CHECK(pixie_project_create(path.c_str()) == PIXIE_OK);
    Session s;
    pixie_session_destroy(s.value); s.value = nullptr;
    CHECK(pixie_session_open_project(96, 64, path.c_str(), &s.value) == PIXIE_OK);
    s.Good("world.hello");
    CHECK(s.Good("world.stats")["mode"] == "edit");
    CHECK(s.Good("world.step", {{"count", 2}})["steps_run"] == 0);
    const auto id = s.Good("world.entities")["entities"][0]["id"];
    const auto token = s.Good("world.snapshot", {{"ids", {id}}})["token"];
    s.Good("world.play");
    CHECK(s.Good("world.step", {{"count", 2}})["steps_run"] == 2);
    CHECK(s.Good("world.entity", {{"id", id}})["id"] == id);
    CHECK(s.Call("world.restore", {{"token", token}}).contains("error"));
    s.Good("world.stop");
    CHECK(s.Good("world.stats")["mode"] == "edit");
    CHECK(s.Good("world.entity", {{"id", id}})["id"] == id);
    s.Good("world.rename", {{"id", id}, {"name", "unsaved change"}});
    CHECK(s.Good("world.stats")["unsaved"] == true);
    CHECK(s.Call("world.save").contains("error"));
    PixieSession* reopened = nullptr;
    CHECK(pixie_session_open_project(96, 64, path.c_str(), &reopened) == PIXIE_OK);
    CHECK(pixie_project_unsaved(reopened) == 0);
    pixie_session_destroy(reopened);
    // Keep the uniquely named test project as evidence. No recursive cleanup.
    std::fprintf(stderr, "Fixture project retained: %s\n", path.c_str());
}

TEST(BorrowedDispatcherLeavesSessionAliveAndDoesNotAssignSceneIds) {
    pixie::ProjectSession session(std::make_unique<pixie::SceneProject>(), {});
    CHECK(session.Start({96, 64}));
    auto procedural = session.GetWorld().Create("procedural");
    const auto count = session.GetWorld().Registry().view<pixie::SceneId>().size();
    {
        pixie::dev::WorldDispatcher dispatcher(session);
        CHECK(Json::parse(dispatcher.Dispatch(Request("world.hello").dump())).contains("result"));
        auto entities = Json::parse(dispatcher.Dispatch(Request("world.entities").dump()));
        CHECK(entities["result"]["value"]["unaddressable_count"].get<int>() >= 1);
        CHECK(session.GetWorld().Registry().view<pixie::SceneId>().size() == count);
        std::string response;
        std::thread worker([&] { response = dispatcher.Dispatch(Request("world.step").dump()); });
        worker.join();
        CHECK(Json::parse(response).contains("error"));
    }
    CHECK(procedural.IsAlive());
    const auto before = session.GetWorld().Steps();
    session.StepOnce();
    CHECK(session.GetWorld().Steps() == before + 1);
    CHECK(session.RenderToTarget() != 0);
    session.Stop();
}

TEST(OverflowAfterMutationReturnsBoundedPossiblyAppliedReceipt) {
    pixie::ProjectSession session(std::make_unique<pixie::SceneProject>(), {});
    CHECK(session.Start({96, 64}));
    auto entity = session.GetWorld().Create("large-native-readback");
    entity.Add<pixie::SceneId>(pixie::SceneId{123});
    auto& layers = entity.Add<pixie::CollisionLayers2D>().layers;
    layers.resize(5);
    for (auto& layer : layers) layer.value.name.assign(900000, 'x');
    pixie::dev::WorldDispatcher dispatcher(session);
    dispatcher.Dispatch(Request("world.hello").dump());
    auto request = Request("world.record_insert");
    request["params"] = {{"id", "000000000000007b"}, {"field", "CollisionLayers2D.layers"}, {"item", 0}};
    const auto response = dispatcher.Dispatch(request.dump());
    CHECK(response.size() < 4096);
    const auto error = Json::parse(response)["error"];
    CHECK(error["data"]["pixie_result"] == PIXIE_ERROR_BUFFER);
    CHECK(error["data"]["possibly_applied"] == true);
    CHECK(layers.size() == 6); // One execution, no implicit retry.
    session.Stop();
}

TEST(RestoreFailureReportsSuccessfulOrFailedRollbackExplicitly) {
    pixie::ProjectSession session(std::make_unique<ProbeProject>(), {});
    CHECK(session.Start({96, 64}));
    pixie::dev::WorldDispatcher dispatcher(session);
    auto call = [&](const char* method, Json params = Json::object()) {
        auto request = Request(method); request["params"] = params;
        return Json::parse(dispatcher.Dispatch(request.dump()));
    };
    call("world.hello");
    const auto id = call("world.entities")["result"]["value"]["entities"][0]["id"];
    const auto token = call("world.snapshot", {{"ids", {id}}})["result"]["value"]["token"];
    call("world.set", {{"id", id}, {"field", "FaultProbe.value"}, {"value", 77}});
    rejectedDefaults = 1;
    const auto first = call("world.restore", {{"token", token}});
    CHECK(first["error"]["data"]["possibly_applied"] == true);
    CHECK(first["error"]["message"].get<std::string>().find("prior live entities restored") != std::string::npos);
    CHECK(call("world.get", {{"id", id}, {"field", "FaultProbe.value"}})["result"]["value"]["value"] == 77);
    rejectedDefaults = 2;
    const auto second = call("world.restore", {{"token", token}});
    CHECK(second["error"]["data"]["possibly_applied"] == true);
    CHECK(second["error"]["message"].get<std::string>().find("rollback failed") != std::string::npos);
    rejectedDefaults = 0;
    CHECK(call("world.restore", {{"token", token}}).contains("result"));
    session.Stop();
}

int main() {
    if (!glfwInit()) return 2;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto* window = glfwCreateWindow(96, 64, "devchannel-tests", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 3; }
    glfwMakeContextCurrent(window);
    if (!pixie::LoadGl()) return 4;
    int result = 1;
    try { result = check::RunAll(); }
    catch (const std::exception& e) { std::fprintf(stderr, "FAIL: %s\n", e.what()); }
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
