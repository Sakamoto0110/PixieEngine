#include "pixie/runtime/ProjectSession.hpp"

#include "pixie/Gl.hpp"
#include "pixie/Log.hpp"
#include "pixie/runtime/ParticleStep.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/SceneProject.hpp"
#include "pixie/runtime/ShapeDrawing.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <fstream>
#include <sstream>

namespace pixie {

ProjectSession::ProjectSession(std::unique_ptr<Project> project, SessionSettings settings)
    : project_(std::move(project)), settings_(settings), stepper_(settings.updateRate) {
    DescribeRuntimeComponents(types_);
    project_->Describe(types_);
}

ProjectSession::~ProjectSession() {
    // The World may hold no GL objects, but the renderers do: Stop must have run.
    world_.reset();
}

bool ProjectSession::OpenProject(const std::filesystem::path& folder) {
    OpenedProject opened = OpenProjectFolder(folder);
    if (!opened.ok) {
        error_ = opened.error;
        return false;
    }
    const std::filesystem::path scenePath = opened.StartScenePath();
    std::ifstream in(scenePath, std::ios::binary);
    if (!in) {
        error_ = std::format("the start scene, {}, could not be opened", opened.file.startScene);
        return false;
    }
    std::ostringstream text;
    text << in.rdbuf();
    // Read once into a World of its own, so a scene that cannot be read is refused here.
    World check;
    if (const SceneReport report = ReadScene(check, types_, text.str()); !report.ok) {
        error_ = std::format("{}: {}", opened.file.startScene, report.error);
        return false;
    }
    images_.SetFolder(opened.folder);
    opened_ = std::move(opened);
    sceneText_ = text.str();
    playedFrom_.clear();
    mode_ = SessionMode::Edit;
    unsaved_ = false;
    if (started_) {
        Build(true);
    }
    return true;
}

std::string_view ProjectSession::ProjectName() const {
    return opened_ ? std::string_view(opened_->file.name) : project_->Name();
}

bool ProjectSession::Start(FramebufferSize size) {
    if (!shapes_.Create() || !particles_.Create() || !sprites_.Create()) {
        return false;
    }
    camera_.SetViewport(size);
    started_ = true;
    Build(true);
    return true;
}

void ProjectSession::Stop() {
    world_.reset();
    target_.Destroy();
    images_.Destroy();
    sprites_.Destroy();
    particles_.Destroy();
    shapes_.Destroy();
    started_ = false;
}

void ProjectSession::Build(bool resetCamera) {
    world_.reset();
    world_ = std::make_unique<World>(settings_.seed);
    if (resetCamera) {
        camera_.SetCenter(pxPoint(0.0, 0.0));
        camera_.SetZoom(1.0);
    }
    if (!sceneText_.empty()) {
        const SceneReport report = ReadScene(*world_, types_, sceneText_);
        if (!report.ok) {
            LogError(std::format("{}: the scene could not be read: {}", ProjectName(), report.error));
        }
        for (const std::string& warning : report.warnings) {
            LogWarning(std::format("{}: {}", ProjectName(), warning));
        }
    }
    project_->Build(*world_, camera_);
}

void ProjectSession::Reset() {
    if (world_) {
        project_->Capture(*world_);
        if (opened_) {
            sceneText_ = WriteScene(*world_, types_);
        }
    }
    Build(!opened_);
    stepper_.Drop();
}

bool ProjectSession::Play() {
    if (!opened_) {
        error_ = "no project is open, so there is no scene to play";
        return false;
    }
    if (mode_ == SessionMode::Play) {
        return true;
    }
    playedFrom_ = WriteScene(*world_, types_);
    sceneText_ = playedFrom_;
    mode_ = SessionMode::Play;
    paused_ = false;
    Build(false);
    stepper_.Drop();
    return true;
}

bool ProjectSession::StopPlaying() {
    if (!opened_) {
        error_ = "no project is open, so there is no scene to go back to";
        return false;
    }
    if (mode_ == SessionMode::Edit) {
        return true;
    }
    sceneText_ = std::move(playedFrom_);
    playedFrom_.clear();
    mode_ = SessionMode::Edit;
    Build(false);
    stepper_.Drop();
    return true;
}

bool ProjectSession::Save() {
    if (!opened_) {
        error_ = "no project is open, so there is nowhere to save";
        return false;
    }
    std::vector<std::string> warnings;
    const std::string text = mode_ == SessionMode::Play ? playedFrom_ : WriteScene(*world_, types_, &warnings);
    for (const std::string& warning : warnings) {
        LogWarning(std::format("{}: {}", ProjectName(), warning));
    }
    if (const SceneReport saved = SaveTextFile(text, opened_->StartScenePath()); !saved.ok) {
        error_ = saved.error;
        return false;
    }
    unsaved_ = false;
    return true;
}

void ProjectSession::MarkChanged() {
    if (opened_ && mode_ == SessionMode::Edit) {
        unsaved_ = true;
    }
}

void ProjectSession::Resize(FramebufferSize size) {
    camera_.SetViewport(size);
}

void ProjectSession::Update(double delta, const Input& input) {
    if (mode_ == SessionMode::Edit) {
        return;
    }
    project_->BeforeStep(*world_, camera_, input);
    const auto start = std::chrono::steady_clock::now();
    world_->Step(delta, input);
    lastStepMilliseconds_ =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

void ProjectSession::Advance(double frameDelta) {
    if (mode_ == SessionMode::Edit) {
        PanAndZoom(camera_, input_);
        input_.EndUpdate();
        return;
    }
    if (paused_) {
        return;
    }
    stepper_.Advance(frameDelta * timeScale_, [this](const FrameTime& time) {
        Update(time.delta, input_);
        input_.EndUpdate();
    });
}

void ProjectSession::StepOnce() {
    if (mode_ == SessionMode::Edit) {
        return;
    }
    Update(stepper_.StepSeconds(), input_);
    input_.EndUpdate();
}

void ProjectSession::SetTimeScale(double scale) {
    timeScale_ = std::clamp(scale, kMinTimeScale, kMaxTimeScale);
}

void ProjectSession::SetPaused(bool paused) {
    paused_ = paused;
    stepper_.Drop();
}

double ProjectSession::Alpha() const {
    return paused_ || mode_ == SessionMode::Edit ? 1.0 : stepper_.RenderTime(0.0, 0).alpha;
}

void ProjectSession::Render(double alpha) {
    const pxColorRgba background = project_->Background();
    glClearColor(background.r / 255.0f, background.g / 255.0f, background.b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (const auto now = std::chrono::steady_clock::now(); now >= nextImageCheck_) {
        images_.CheckForChanges();
        nextImageCheck_ = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                    std::chrono::duration<double>(SpriteImages::kCheckInterval));
    }
    SubmitSprites(*world_, sprites_, images_, alpha);
    sprites_.Flush(camera_);
    DrawParticles(*world_, particles_, camera_, alpha);
    SubmitShapes(*world_, shapes_, alpha);
    project_->DrawOverlay(*world_, shapes_, camera_);
    shapes_.Flush(camera_);
}

unsigned ProjectSession::RenderToTarget() {
    const FramebufferSize size = camera_.Viewport();
    if (size.width <= 0 || size.height <= 0 || !target_.Resize(size)) {
        return 0;
    }
    target_.Bind();
    Render(Alpha());
    RenderTarget::BindDefault();
    return target_.ColorTexture();
}

} // namespace pixie
