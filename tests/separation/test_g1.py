"""Focused executor orchestration controls; all subprocesses are mocked."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.dont_write_bytecode=True
import g1
from artifacts import InputError, absolute_path, digest
WORK=None
FIXTURE=None

def write(path,value):
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
    return digest(path)

class ExecutorTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(WORK)
        self.temporary=tempfile.TemporaryDirectory(prefix='g1-controls-',dir=WORK)
        self.root=Path(self.temporary.name).absolute()
        self.assertTrue(self.root.is_relative_to(WORK))
        self.addCleanup(self.temporary.cleanup)
        self.sdk=self.root/'SDK ação';self.sdk.mkdir()
        self.fixture=self.root/'fixture-source';self.fixture.mkdir()
        for name in g1.FIXTURE_FILES:(self.fixture/name).write_bytes((FIXTURE/name).read_bytes())
        runtime=self.sdk/'profiles/authoring/Release/bin';runtime.mkdir(parents=True)
        (runtime/'pixie.dll').write_bytes(b'not a native binary')
        (runtime/'glfw3.dll').write_bytes(b'not GLFW')
        profile={'schemaVersion':1,'cApiVersion':28,'moduleIdentityContract':1,'platform':'windows',
                 'architecture':'x64','compilerId':'MSVC','compilerVersion':'synthetic','msvcFullVersion':1,
                 'stlVersion':1,'stlUpdate':1,'iteratorDebugLevel':0,'cppStandard':20,'crt':'MD',
                 'cpuFeatures':['avx2','fma','f16c'],'distributionClass':'public','exceptions':True,'rtti':True,
                 'windowsSdkVersion':'synthetic','dependencyInventorySha256':'c'*64,'sdkId':'a'*64,'sdkBuildId':'b'*64,'profile':'authoring','configuration':'Release',
                 'runtime':{'path':'profiles/authoring/Release/bin/pixie.dll','sha256':digest(runtime/'pixie.dll')},
                 'nativeDependencies':[{'path':'profiles/authoring/Release/bin/glfw3.dll','sha256':digest(runtime/'glfw3.dll')}]}
        profile_path=self.sdk/'profiles/authoring/Release/profile.json';profile_hash=write(profile_path,profile)
        manifest={'schemaVersion':1,'sdkId':'a'*64,'profiles':[{'profile':'authoring','configuration':'Release',
                  'sdkBuildId':'b'*64,'manifest':'profiles/authoring/Release/profile.json','manifestSha256':profile_hash}],
                  'files':[{'path':p.relative_to(self.sdk).as_posix(),'size':p.stat().st_size,'sha256':digest(p)}
                           for p in sorted(self.sdk.rglob('*')) if p.is_file()]}
        manifest_hash=write(self.sdk/'sdk.json',manifest)
        fake_tool=self.root/'not-executed-tool.exe';fake_tool.write_bytes(b'synthetic tool; subprocess is mocked')
        environment={'values':{'PATH':'synthetic-only','INCLUDE':'synthetic-only','LIB':'synthetic-only'}}
        env_path=self.root/'environment.json';env_hash=write(env_path,environment)
        provenance_path=self.root/'provenance.json';prov_hash=write(provenance_path,{'inputKind':'synthetic-control','scope':'synthetic-only'})
        self.data={'schema':'pixie-separation-focused-g1-execution/1','draft':False,'runId':'synthetic-only',
            'evidenceLevel':'focused-only','inputKind':'synthetic-control','selection':{'PIXIE_SDK_ROOT':str(self.sdk),'sdkManifestSha256':manifest_hash,
            'sdkId':'a'*64,'sdkBuildId':'b'*64,'profileManifestSha256':profile_hash,
            'PIXIE_SDK_PROFILE':'authoring','PIXIE_CONSUMER_CONFIGURATION':'Release','PIXIE_CONSUMER_RUNTIME_DIR':str(runtime)},
            'fixture':{'root':str(self.fixture),'files':{name:digest(self.fixture/name) for name in g1.FIXTURE_FILES}},
            'tools':{name:{'path':str(fake_tool),'sha256':digest(fake_tool),'version':'synthetic-only'}
                     for name in ('cmake','ctest','compiler','generator')},'generatorName':'Ninja',
            'toolchainEnvironment':{'values':environment['values'],'receiptPath':str(env_path),'receiptSha256':env_hash},
            'provenance':{'receiptPath':str(provenance_path),'receiptSha256':prov_hash,'archivePath':None},
            'outputs':{'ownedRoot':str(self.root)},'limits':{'configureSeconds':30,'buildSeconds':30,'testSeconds':30}}

    def problem(self,code,operation):
        with self.assertRaises(InputError) as failure:operation()
        self.assertEqual(failure.exception.code,code)

    def completed(self,code=0):return subprocess.CompletedProcess([],code,stdout=b'synthetic-only stdout',stderr=b'')

    def successful_child(self, arguments, **kwargs):
        run=Path(kwargs['cwd'])
        if '--test-dir' in arguments:
            self.create_receipts(run)
        return self.completed()

    def create_receipts(self,run,change=None):
        stage=run/'build/bin/Release';stage.mkdir(parents=True,exist_ok=True)
        source=self.sdk/'profiles/authoring/Release/bin'
        for name in ['pixie.dll','glfw3.dll']:(stage/name).write_bytes((source/name).read_bytes())
        (stage/'pixie-installed-runtime.exe').write_bytes(b'synthetic mocked executable; never executed')
        (run/'build/CheckConsumer-Release.cmake').write_text('# synthetic mocked generated checker')
        profile=json.loads((self.sdk/'profiles/authoring/Release/profile.json').read_text())
        expected={'sdkId':'a'*64,'sdkBuildId':'b'*64,'profile':'authoring','configuration':'Release'}
        for name,mode,count in [('identity.stdout.json','identity-only',11),('consumer.stdout.json','headless-runtime',42)]:
            packet={'fixture':'pixie-installed-headless/1','mode':mode,'checks':count,
                    'loadedRuntime':str(stage/'pixie.dll'),'runtimeIdentity':dict(profile),'expected':expected}
            if change is not None:change(name,packet)
            write(stage/name,packet)
        for name in ['identity.stderr.txt','consumer.stderr.txt']:(stage/name).write_bytes(b'')

    def test_ready_inputs_validate_without_launch(self):
        with mock.patch.object(g1.subprocess,'run') as child:
            g1.validate(self.data)
            child.assert_not_called()

    def test_phase_order_and_no_product_pass(self):
        with mock.patch.object(g1.subprocess,'run',side_effect=self.successful_child) as child:
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual([phase['stage'] for phase in result['phases']],['configure','build','test'])
        self.assertEqual(result['status'],'synthetic-control-pass')
        self.assertFalse(result['productGatePassed'])
        self.assertEqual(result['G1'],'blocked')
        self.assertEqual(child.call_count,3)
        self.assertTrue(all(call.kwargs['cwd']==self.root/'run' for call in child.call_args_list))
        self.assertTrue(all('PIXIE_ENGINE_DIR' not in call.kwargs['env'] for call in child.call_args_list))
        self.assertFalse(any(p.name=='PixieEngine' for p in (self.root/'run').iterdir()))

    def test_configure_failure_stops_build_and_test(self):
        with mock.patch.object(g1.subprocess,'run',return_value=self.completed(1)) as child:
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(child.call_count,1)
        self.assertEqual(result['phases'][0]['status'],'fail')
        self.assertFalse(result['productGatePassed'])

    def test_build_failure_stops_test(self):
        with mock.patch.object(g1.subprocess,'run',side_effect=[self.completed(),self.completed(9)]) as child:
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(child.call_count,2)
        self.assertEqual(result['status'],'incomplete')

    def test_timeout_does_not_claim_child_tree_termination(self):
        with mock.patch.object(g1.subprocess,'run',side_effect=subprocess.TimeoutExpired(['synthetic'],30)) as child:
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(child.call_count,1)
        self.assertEqual(result['phases'][0]['reason'],'timeout')
        self.assertFalse(result['phases'][0]['processTreeTerminationVerified'])

    def test_draft_refused_before_child(self):
        self.data['draft']=True
        with mock.patch.object(g1.subprocess,'run') as child:
            self.problem('draft-input',lambda:g1.execute(self.data,str(self.root/'run')))
        child.assert_not_called()

    def test_sdk_file_drift_refused(self):
        (self.sdk/'profiles/authoring/Release/bin/pixie.dll').write_bytes(b'changed')
        self.problem('inventory-mismatch',lambda:g1.validate(self.data))

    def test_selected_tuple_mismatch_refused(self):
        self.data['selection']['sdkBuildId']='f'*64
        self.problem('selection-mismatch',lambda:g1.validate(self.data))

    def test_extra_fixture_file_refused(self):
        (self.fixture/'producer.cpp').write_bytes(b'unwanted')
        self.problem('fixture-payload',lambda:g1.validate(self.data))

    def test_tool_hash_refused(self):
        self.data['tools']['compiler']['sha256']='f'*64
        self.problem('hash-mismatch',lambda:g1.validate(self.data))

    def test_environment_receipt_mismatch(self):
        self.data['toolchainEnvironment']['values']['PATH']='different'
        self.problem('environment-mismatch',lambda:g1.validate(self.data))

    def test_output_escape_and_reuse_refused(self):
        self.problem('output-scope',lambda:g1.execute(self.data,str(self.root.parent/'escape')))
        (self.root/'existing').mkdir()
        self.problem('output-scope',lambda:g1.execute(self.data,str(self.root/'existing')))

    def test_sdk_change_after_configure_stops_build(self):
        def change(*args,**kwargs):
            (self.sdk/'unexpected.txt').write_text('changed')
            return self.completed()
        with mock.patch.object(g1.subprocess,'run',side_effect=change) as child:
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(child.call_count,1)
        self.assertEqual(result['status'],'incomplete')
        self.assertEqual(result['phases'][0]['reason'],'input-drift')
        self.assertTrue((self.root/'run/results.json').is_file())

    def test_command_shape_uses_final_f1_names(self):
        plan=g1.commands(self.data,self.root/'run')
        configure=plan[0][1]
        self.assertTrue(any(arg.startswith('-DPIXIE_SDK_ROOT=') for arg in configure))
        self.assertTrue(any(arg.startswith('-DPIXIE_SDK_PROFILE=') for arg in configure))
        self.assertTrue(any(arg.startswith('-DCMAKE_BUILD_TYPE=Release') for arg in configure))
        self.assertFalse(any('PIXIE_SDK_CONFIGURATION=' in arg for arg in configure))
        self.assertFalse(any('add_subdirectory' in arg for arg in configure))

    def test_ctest_exact_filter_and_zero_test_error(self):
        test=g1.commands(self.data,self.root/'run')[-1][1]
        self.assertEqual(test[test.index('-R')+1],'^sdk-installed-headless$')
        self.assertIn('--no-tests=error',test)

    def test_three_zero_exits_without_actual_receipts_fail(self):
        with mock.patch.object(g1.subprocess,'run',return_value=self.completed()) as child:
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(child.call_count,3)
        self.assertEqual(result['status'],'incomplete')
        self.assertEqual(result['phases'][-1]['reason'],'fixture-receipt-missing')
        self.assertFalse(result['productGatePassed'])

    def test_zero_tests_exit_stops_without_success(self):
        with mock.patch.object(g1.subprocess,'run',side_effect=[self.completed(),self.completed(),self.completed(8)]):
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(result['status'],'incomplete')
        self.assertEqual(result['phases'][-1]['exitCode'],8)
        self.assertEqual(result['fixtureReceipts']['status'],'unverified')

    def test_launch_error_retains_incomplete_final_receipt(self):
        with mock.patch.object(g1.subprocess,'run',side_effect=OSError('simulated launch failure')) as child:
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(child.call_count,1)
        retained=json.loads((self.root/'run/results.json').read_text())
        self.assertEqual(retained['status'],'incomplete')
        self.assertEqual(retained['phases'][0]['reason'],'process-or-io-error')
        self.assertEqual(result['status'],'incomplete')

    def test_stale_receipt_prevents_test_launch(self):
        def child(arguments,**kwargs):
            if '--build' in arguments:self.create_receipts(Path(kwargs['cwd']))
            return self.completed()
        with mock.patch.object(g1.subprocess,'run',side_effect=child) as mocked:
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(mocked.call_count,2)
        self.assertEqual(result['phases'][-1]['reason'],'stale-fixture-receipt')

    def test_receipt_wrong_mode_and_typed_identity(self):
        def change(name,packet):
            if name=='consumer.stdout.json':packet['checks']='42'
        def child(arguments,**kwargs):
            if '--test-dir' in arguments:self.create_receipts(Path(kwargs['cwd']),change)
            return self.completed()
        with mock.patch.object(g1.subprocess,'run',side_effect=child):
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(result['phases'][-1]['reason'],'fixture-receipt-invalid')
        self.assertTrue((self.root/'run/fixture-receipts/consumer.stdout.json').is_file())

    def test_receipt_wrong_native_boolean_type(self):
        def change(name,packet):packet['runtimeIdentity']['exceptions']=1
        def child(arguments,**kwargs):
            if '--test-dir' in arguments:self.create_receipts(Path(kwargs['cwd']),change)
            return self.completed()
        with mock.patch.object(g1.subprocess,'run',side_effect=child):
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(result['phases'][-1]['reason'],'fixture-receipt-identity')

    def test_failed_test_retains_partial_fixture_receipts(self):
        def child(arguments,**kwargs):
            if '--test-dir' in arguments:
                self.create_receipts(Path(kwargs['cwd']))
                (Path(kwargs['cwd'])/'build/bin/Release/consumer.stdout.json').unlink()
                return self.completed(8)
            return self.completed()
        with mock.patch.object(g1.subprocess,'run',side_effect=child):
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(result['status'],'incomplete')
        self.assertEqual(result['fixtureReceipts']['status'],'unverified')
        self.assertIn('consumer.stdout.json',result['fixtureReceipts']['missing'])
        self.assertTrue((self.root/'run/fixture-receipts/identity.stdout.json').is_file())
        self.assertTrue((self.root/'run/results.json').is_file())

    def test_missing_executable_or_checker_cannot_be_runtime_evidence(self):
        def child(arguments,**kwargs):
            if '--test-dir' in arguments:
                self.create_receipts(Path(kwargs['cwd']))
                (Path(kwargs['cwd'])/'build/bin/Release/pixie-installed-runtime.exe').unlink()
            return self.completed()
        with mock.patch.object(g1.subprocess,'run',side_effect=child):
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(result['status'],'incomplete')
        self.assertEqual(result['phases'][-1]['reason'],'fixture-artifact-missing')

    def test_staged_native_dependency_tampering_is_rejected(self):
        def child(arguments,**kwargs):
            if '--test-dir' in arguments:
                self.create_receipts(Path(kwargs['cwd']))
                (Path(kwargs['cwd'])/'build/bin/Release/glfw3.dll').write_bytes(b'changed')
            return self.completed()
        with mock.patch.object(g1.subprocess,'run',side_effect=child):
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(result['phases'][-1]['reason'],'fixture-runtime-closure')

    def test_malformed_provenance_and_environment_are_attributable(self):
        path=Path(self.data['provenance']['receiptPath'])
        self.data['provenance']['receiptSha256']=write(path,[])
        self.problem('provenance-schema',lambda:g1.validate(self.data))
        self.data['provenance']['receiptSha256']=write(path,{'inputKind':'synthetic-control'})
        path=Path(self.data['toolchainEnvironment']['receiptPath'])
        self.data['toolchainEnvironment']['receiptSha256']=write(path,[])
        self.problem('environment-mismatch',lambda:g1.validate(self.data))

    def test_boolean_root_schema_is_not_integer_one(self):
        path=self.sdk/'sdk.json';manifest=json.loads(path.read_text());manifest['schemaVersion']=True
        self.data['selection']['sdkManifestSha256']=write(path,manifest)
        self.problem('selection-mismatch',lambda:g1.validate(self.data))

    def test_actual_provenance_absence_is_explicit_and_not_run(self):
        self.data['inputKind']='producer-artifact'
        with mock.patch.object(g1.subprocess,'run',side_effect=self.successful_child) as child:
            result=g1.execute(self.data,str(self.root/'run'))
        child.assert_not_called()
        self.assertEqual(result['status'],'incomplete')
        self.assertEqual(result['provenance']['status'],'incomplete')
        self.assertTrue(result['provenance']['issues'])
        self.assertEqual(result['issues'][0]['code'],'provenance-incomplete')

    def test_producer_provenance_source_and_dirty_bytes_are_checked(self):
        self.data['inputKind']='producer-artifact'
        source=self.root/'actual-input-inventory.tsv';source.write_text('source.cpp\t'+'1'*64+'\n')
        dirty=self.root/'actual-dirty-inventory.tsv';dirty.write_text('source.cpp\t'+'2'*64+'\n')
        provenance={'inputKind':'producer-artifact','sdkId':'a'*64,'sdkBuildId':'b'*64,'producerHead':'d'*40,
                    'dependencyInventorySha256':'c'*64,'sourceInventorySha256':digest(source),'dirtyManifestSha256':digest(dirty)}
        self.data['provenance'].update(sourceInventory={'path':str(source),'sha256':digest(source)},
            dirtyManifest={'path':str(dirty),'sha256':digest(dirty)})
        self.data['provenance']['receiptSha256']=write(Path(self.data['provenance']['receiptPath']),provenance)
        context=g1.validate(self.data)
        self.assertFalse(context['provenanceIssues'])
        source.write_text('changed')
        self.problem('hash-mismatch',lambda:g1.validate(self.data))

    def test_copied_fixture_drift_preserves_incomplete_result(self):
        def child(arguments,**kwargs):
            (Path(kwargs['cwd'])/'fixture/InstalledRuntime.cpp').write_text('changed after phase')
            return self.completed()
        with mock.patch.object(g1.subprocess,'run',side_effect=child) as mocked:
            result=g1.execute(self.data,str(self.root/'run'))
        self.assertEqual(mocked.call_count,1)
        self.assertEqual(result['status'],'incomplete')
        self.assertEqual(result['phases'][0]['reason'],'hash-mismatch')
        self.assertTrue((self.root/'run/results.json').is_file())

if __name__=='__main__':
    parser=argparse.ArgumentParser(add_help=False);parser.add_argument('--work',required=True);parser.add_argument('--fixture',required=True);options,remaining=parser.parse_known_args()
    WORK=absolute_path(options.work);FIXTURE=absolute_path(options.fixture)
    unittest.main(argv=[sys.argv[0],*remaining])
