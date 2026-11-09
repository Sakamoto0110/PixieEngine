"""Focused installed-G1 command adapter; product-gate acceptance is separate.

Only explicit C2 selections are consumed. Native phase success remains focused
input for independent review, with productGatePassed=False and no G1 closure.
"""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import time

sys.dont_write_bytecode = True
from artifacts import (InputError, absolute_path, canonical_hash, digest, file_reference,
                       inventory, read_json, require, sha256, tree_files)

NATIVE_MATCH_FIELDS = (
    'schemaVersion', 'sdkId', 'sdkBuildId', 'cApiVersion', 'moduleIdentityContract',
    'platform', 'architecture', 'compilerId', 'compilerVersion', 'msvcFullVersion',
    'stlVersion', 'stlUpdate', 'iteratorDebugLevel', 'cppStandard', 'configuration',
    'crt', 'cpuFeatures', 'profile', 'distributionClass', 'exceptions', 'rtti',
    'windowsSdkVersion', 'dependencyInventorySha256')
FIXTURE_RECEIPTS = ('identity.stdout.json', 'identity.stderr.txt', 'consumer.stdout.json', 'consumer.stderr.txt')
FIXTURE_FILES = {'CMakeLists.txt', 'InstalledRuntime.cpp'}
ENVIRONMENT_KEYS = {'PATH', 'INCLUDE', 'LIB', 'LIBPATH', 'WindowsSdkDir', 'WindowsSDKVersion',
                    'WindowsLibPath', 'VCToolsInstallDir', 'VCToolsVersion', 'VCINSTALLDIR',
                    'VSINSTALLDIR', 'VisualStudioVersion', 'VSCMD_ARG_HOST_ARCH', 'VSCMD_ARG_TGT_ARCH'}

def write_json(path, value):
    with path.open('x', encoding='utf-8', newline='\n') as output:
        json.dump(value, output, indent=2, ensure_ascii=False, allow_nan=False)
        output.write('\n')

def validate(data):
    require(isinstance(data, dict) and data.get('draft') is False, 'draft-input',
            'A complete nondraft execution input is required', blocked=True)
    require(data.get('schema') == 'pixie-separation-focused-g1-execution/1'
            and data.get('evidenceLevel') == 'focused-only', 'execution-schema',
            'Only the focused G1 execution interface is supported')
    selected = data['selection']
    root = absolute_path(selected['PIXIE_SDK_ROOT'])
    manifest_path = file_reference(str(root/'sdk.json'), selected['sdkManifestSha256'])
    manifest = read_json(manifest_path)
    require(isinstance(manifest,dict),'manifest-schema','C2 root manifest must be an object')
    sha256(selected['sdkId']);sha256(selected['sdkBuildId'])
    require(type(manifest.get('schemaVersion')) is int and manifest['schemaVersion'] == 1 and manifest.get('sdkId') == selected['sdkId'],
            'selection-mismatch', 'SDK root identity differs from explicit selection')
    entries = [entry for entry in manifest.get('profiles', [])
               if entry.get('profile') == selected['PIXIE_SDK_PROFILE']
               and entry.get('configuration') == selected['PIXIE_CONSUMER_CONFIGURATION']]
    require(len(entries) == 1 and entries[0].get('sdkBuildId') == selected['sdkBuildId'],
            'selection-mismatch', 'Selected SDK profile/config/build must exist exactly once')
    profile_relative = f"profiles/{selected['PIXIE_SDK_PROFILE']}/{selected['PIXIE_CONSUMER_CONFIGURATION']}/profile.json"
    require(entries[0].get('manifest') == profile_relative
            and entries[0].get('manifestSha256') == selected['profileManifestSha256'],
            'selection-mismatch', 'Selected profile path/hash mismatch')
    profile_path = file_reference(str(root/profile_relative), selected['profileManifestSha256'])
    profile = read_json(profile_path)
    require(isinstance(profile,dict),'manifest-schema','C2 profile manifest must be an object')
    require(all(profile.get(key) == expected for key, expected in
                [('sdkId', selected['sdkId']), ('sdkBuildId', selected['sdkBuildId']),
                 ('profile', selected['PIXIE_SDK_PROFILE']), ('configuration', selected['PIXIE_CONSUMER_CONFIGURATION'])]),
            'selection-mismatch', 'Profile identity differs from selection')
    declared = inventory({'schema':'pixie-artifact-inventory/1', 'files':[
        {key: entry[key] for key in ('path', 'size', 'sha256')} for entry in manifest['files']]})
    actual = tree_files(str(root))
    actual.pop('sdk.json', None)
    require(actual == declared, 'inventory-mismatch', 'Actual SDK tree differs from pinned C2 inventory')
    require(data.get('inputKind') in ('synthetic-control','producer-artifact'), 'input-kind',
            'Declare synthetic-control or producer-artifact explicitly')
    fixture = absolute_path(data['fixture']['root'])
    require(set(data['fixture']['files']) == FIXTURE_FILES
            and set(tree_files(str(fixture))) == FIXTURE_FILES,
            'fixture-payload', 'Exactly the hash-bound F1 two-file fixture is required')
    references = [(manifest_path, selected['sdkManifestSha256']), (profile_path, selected['profileManifestSha256'])]
    for name in sorted(FIXTURE_FILES):
        references.append((file_reference(str(fixture/name), data['fixture']['files'][name]), data['fixture']['files'][name]))
    for name in ('cmake', 'ctest', 'compiler', 'generator'):
        tool = data['tools'][name]
        require(isinstance(tool.get('version'), str) and tool['version'], 'tool-version',
                'An independently recorded tool version is required', blocked=True)
        references.append((file_reference(tool.get('path'), tool.get('sha256')), tool['sha256']))
    require(data['generatorName'] in ('Ninja', 'Ninja Multi-Config'), 'unsupported-generator',
            'Initial focused adapter supports declared Ninja generators only')
    environment = data['toolchainEnvironment']
    env_path = file_reference(environment['receiptPath'], environment['receiptSha256'])
    environment_document=read_json(env_path)
    require(isinstance(environment_document,dict),'environment-mismatch','Toolchain receipt must be an object')
    require(isinstance(environment['values'], dict) and set(environment['values']) <= ENVIRONMENT_KEYS
            and all(isinstance(value, str) for value in environment['values'].values())
            and environment_document.get('values') == environment['values'],
            'environment-mismatch', 'Toolchain environment is not exactly bound to its receipt')
    references.append((env_path, environment['receiptSha256']))
    provenance = data['provenance']
    provenance_path = file_reference(provenance['receiptPath'], provenance['receiptSha256'])
    references.append((provenance_path, provenance['receiptSha256']))
    provenance_document = read_json(provenance_path)
    require(isinstance(provenance_document,dict),'provenance-schema','Provenance receipt must be a JSON object')
    provenance_issues = []
    if data['inputKind'] == 'synthetic-control':
        require(provenance_document.get('inputKind') == 'synthetic-control', 'provenance-kind',
                'Synthetic controls require an explicitly synthetic provenance receipt')
    else:
        for key in ('sdkId','sdkBuildId'):
            if provenance_document.get(key) != selected[key]:
                provenance_issues.append('provenance selection mismatch: '+key)
        head = provenance_document.get('producerHead')
        if not isinstance(head,str) or re.fullmatch(r'[0-9a-f]{40}|[0-9a-f]{64}',head) is None:
            provenance_issues.append('missing/invalid actual producerHead')
        dependency = provenance_document.get('dependencyInventorySha256')
        if not isinstance(dependency,str) or re.fullmatch(r'[0-9a-f]{64}',dependency) is None or dependency != profile.get('dependencyInventorySha256'):
            provenance_issues.append('missing/mismatched actual dependency inventory identity')
        for name, field in [('sourceInventory','sourceInventorySha256'),('dirtyManifest','dirtyManifestSha256')]:
            reference = provenance.get(name)
            if not isinstance(reference,dict) or not reference.get('path') or not reference.get('sha256'):
                provenance_issues.append('actual '+name+' artifact reference is missing')
                continue
            if provenance_document.get(field) != reference['sha256']:
                provenance_issues.append('actual '+name+' digest is not bound by provenance')
                continue
            references.append((file_reference(reference['path'],reference['sha256']),reference['sha256']))
    if provenance.get('archivePath') is not None:
        references.append((file_reference(provenance['archivePath'], provenance['archiveSha256']), provenance['archiveSha256']))
    runtime_dir = absolute_path(selected['PIXIE_CONSUMER_RUNTIME_DIR'])
    require(runtime_dir.is_dir(), 'missing-runtime', 'Explicit declared runtime directory is missing', blocked=True)
    for reference in [profile['runtime'], *profile.get('nativeDependencies', [])]:
        payload = Path(reference['path']).name
        references.append((file_reference(str(runtime_dir/payload), reference['sha256']), reference['sha256']))
    return {'sdkRoot':root, 'fixtureRoot':fixture, 'manifest':manifest, 'profile':profile,
            'declared':declared, 'references':references, 'inputKind':data['inputKind'],
            'provenanceDocument':provenance_document, 'provenanceIssues':provenance_issues}

def recheck(context):
    for path, expected in context['references']:
        file_reference(str(path), expected)
    actual = tree_files(str(context['sdkRoot']))
    actual.pop('sdk.json', None)
    require(actual == context['declared'], 'input-drift', 'SDK tree changed around a native phase')

def commands(data, run):
    selected, tools = data['selection'], data['tools']
    configure = [tools['cmake']['path'], '-S', str(run/'fixture'), '-B', str(run/'build'),
                 '-G', data['generatorName'], '-DCMAKE_CXX_COMPILER='+tools['compiler']['path'],
                 '-DCMAKE_MAKE_PROGRAM='+tools['generator']['path'],
                 '-DPIXIE_SDK_ROOT='+selected['PIXIE_SDK_ROOT'],
                 '-DPIXIE_SDK_PROFILE='+selected['PIXIE_SDK_PROFILE'],
                 '-DPIXIE_CONSUMER_CONFIGURATION='+selected['PIXIE_CONSUMER_CONFIGURATION'],
                 '-DCMAKE_BUILD_TYPE='+selected['PIXIE_CONSUMER_CONFIGURATION'],
                 '-DPIXIE_CONSUMER_EXPECT_SDK_ID='+selected['sdkId'],
                 '-DPIXIE_CONSUMER_EXPECT_BUILD_ID='+selected['sdkBuildId'],
                 '-DPIXIE_CONSUMER_EXPECT_MANIFEST_SHA256='+selected['sdkManifestSha256'],
                 '-DPIXIE_CONSUMER_RUNTIME_DIR='+selected['PIXIE_CONSUMER_RUNTIME_DIR']]
    return [('configure',configure),
            ('build',[tools['cmake']['path'],'--build',str(run/'build'),'--config',selected['PIXIE_CONSUMER_CONFIGURATION'],'--target','pixie-installed-runtime']),
            ('test',[tools['ctest']['path'],'--test-dir',str(run/'build'),'-C',selected['PIXIE_CONSUMER_CONFIGURATION'],'-R','^sdk-installed-headless$',
                     '--no-tests=error','--output-on-failure'])]

def same_typed(left,right):
    if type(left) is not type(right):return False
    if isinstance(left,dict):return left.keys()==right.keys() and all(same_typed(left[k],right[k]) for k in left)
    if isinstance(left,list):return len(left)==len(right) and all(same_typed(a,b) for a,b in zip(left,right))
    return left==right

def retain_fixture_receipts(data,run):
    stage=run/'build'/'bin'/data['selection']['PIXIE_CONSUMER_CONFIGURATION']
    retained=absolute_path(run/'fixture-receipts');retained.mkdir()
    files={};missing=[]
    for name in FIXTURE_RECEIPTS:
        source=absolute_path(stage/name)
        if not source.is_file():
            missing.append(name)
            continue
        shutil.copyfile(source,retained/name)
        files[name]={'path':str(retained/name),'sha256':digest(retained/name)}
    return {'status':'unverified','files':files,'missing':missing}

def fixture_receipts(data,context,run,retention):
    stage=run/'build'/'bin'/data['selection']['PIXIE_CONSUMER_CONFIGURATION']
    retained=run/'fixture-receipts';files=dict(retention['files'])
    require(not retention['missing'],'fixture-receipt-missing','Expected test produced no receipt: '+','.join(retention['missing']))
    expected_selection={'sdkId':data['selection']['sdkId'],'sdkBuildId':data['selection']['sdkBuildId'],
        'profile':data['selection']['PIXIE_SDK_PROFILE'],'configuration':data['selection']['PIXIE_CONSUMER_CONFIGURATION']}
    for name,mode,count in [('identity.stdout.json','identity-only',11),('consumer.stdout.json','headless-runtime',42)]:
        packet=read_json(retained/name)
        require(isinstance(packet,dict) and packet.get('fixture')=='pixie-installed-headless/1'
                and packet.get('mode')==mode and type(packet.get('checks')) is int and packet['checks']==count,
                'fixture-receipt-invalid','Unexpected fixture mode/check count: '+name)
        require(packet.get('expected')==expected_selection,'fixture-receipt-selection','Fixture header selection differs from pinned inputs')
        identity=packet.get('runtimeIdentity')
        require(isinstance(identity,dict),'fixture-receipt-identity','Missing actual C1 identity')
        for field in NATIVE_MATCH_FIELDS:
            require(field in identity and field in context['profile']
                    and same_typed(identity[field],context['profile'][field]),
                    'fixture-receipt-identity','Typed C1 receipt mismatch: '+field)
        loaded=absolute_path(packet.get('loadedRuntime'))
        require(loaded.is_file() and loaded.samefile(stage/'pixie.dll')
                and digest(loaded)==context['profile']['runtime']['sha256'],
                'fixture-receipt-runtime','Fixture loaded an unselected runtime file')
    for reference in [context['profile']['runtime'],*context['profile'].get('nativeDependencies',[])]:
        payload=absolute_path(stage/Path(reference['path']).name)
        require(payload.is_file() and digest(payload)==reference['sha256'],
                'fixture-runtime-closure','Staged declared runtime dependency changed')
        files[payload.name]={'path':str(payload),'sha256':digest(payload)}
    for payload in [stage/'pixie-installed-runtime.exe',run/'build'/('CheckConsumer-'+data['selection']['PIXIE_CONSUMER_CONFIGURATION']+'.cmake')]:
        require(payload.is_file(),'fixture-artifact-missing','Missing actual fixture executable/checker: '+str(payload))
        files[payload.name]={'path':str(payload),'sha256':digest(payload)}
    return {'status':'verified','files':files,'inputKind':context['inputKind'],
            'meaning':'Synthetic controls only' if context['inputKind']=='synthetic-control' else 'Focused actual fixture receipts; no isolation/product acceptance'}

def execute(data, output, input_reference=None):
    context = validate(data)
    if input_reference is not None:
        context['references'].append((file_reference(str(input_reference[0]),input_reference[1]),input_reference[1]))
    owned, run = absolute_path(data['outputs']['ownedRoot']), absolute_path(output)
    require(owned.is_dir() and run != owned and run.is_relative_to(owned) and not run.exists()
            and run.parent.is_dir(), 'output-scope', 'Use a new child of the existing explicit owned root')
    require(not any(run.is_relative_to(path) or path.is_relative_to(run)
                    for path in (context['sdkRoot'],context['fixtureRoot'],*(p for p,_ in context['references']))),
            'output-overlap', 'Output overlaps immutable inputs')
    run.mkdir();(run/'logs').mkdir()
    result={'schema':'pixie-separation-focused-g1-result/1','runId':data['runId'], 'status':'incomplete',
            'inputKind':context['inputKind'],'phases':[], 'issues':[], 'fixtureReceipts':{'status':'not-run'},
            'provenance':{'status':'incomplete' if context['provenanceIssues'] else 'verified',
                          'issues':context['provenanceIssues'],'document':context['provenanceDocument']},
            'productGatePassed':False,'G1':'blocked','sourceUnavailable':'not-run',
            'limit':'Native tool success is focused only; source-unavailability and coordinator acceptance are separate.'}
    try:
        write_json(run/'inputs.snapshot.json',data)
        result['inputsSnapshotSha256']=digest(run/'inputs.snapshot.json')
        if context['provenanceIssues']:
            raise InputError('provenance-incomplete','Actual source/dirty/dependency provenance is incomplete',blocked=True)
        (run/'fixture').mkdir();(run/'temporary').mkdir()
        for name in sorted(FIXTURE_FILES):
            destination=run/'fixture'/name
            shutil.copyfile(context['fixtureRoot']/name,destination)
            require(digest(destination)==data['fixture']['files'][name],'input-drift','Fixture copy changed')
            context['references'].append((destination,data['fixture']['files'][name]))
        environment={key:value for key,value in os.environ.items() if key.upper() in ('SYSTEMROOT','WINDIR','COMSPEC')}
        environment.update(data['toolchainEnvironment']['values'])
        environment.update(TEMP=str(run/'temporary'),TMP=str(run/'temporary'),PYTHONDONTWRITEBYTECODE='1')
        write_json(run/'environment.json',{'values':environment,'scope':'declared child process environment only'})
        for name,arguments in commands(data,run):
            record={'stage':name,'argv':arguments,'cwd':str(run),'environmentSha256':canonical_hash(environment),
                    'status':'blocked','exitCode':None}
            stdout=stderr=b'';started=time.monotonic()
            try:
                recheck(context)
                if name=='test':
                    stage=run/'build'/'bin'/data['selection']['PIXIE_CONSUMER_CONFIGURATION']
                    require(not any((stage/name).exists() for name in FIXTURE_RECEIPTS),
                            'stale-fixture-receipt','Fixture receipts existed before the selected test ran')
                timeout=data['limits'][name+'Seconds']
                require(type(timeout) is int and 1<=timeout<=1800,'invalid-timeout','Bounded phase timeout required')
                record['timeoutSeconds']=timeout
                completed=subprocess.run(arguments,cwd=run,env=environment,capture_output=True,timeout=timeout)
                stdout,stderr=completed.stdout,completed.stderr
                record.update(exitCode=completed.returncode,status='pass' if completed.returncode==0 else 'fail')
                if name=='test':
                    result['fixtureReceipts']=retain_fixture_receipts(data,run)
                if record['status']=='pass':
                    recheck(context)
                    if name=='test':result['fixtureReceipts']=fixture_receipts(data,context,run,result['fixtureReceipts'])
            except subprocess.TimeoutExpired as error:
                record.update(status='blocked',reason='timeout',processTreeTerminationVerified=False)
                stdout,stderr=error.stdout or b'',error.stderr or b''
            except (InputError,OSError) as error:
                record.update(status='blocked' if isinstance(error,InputError) and error.blocked else 'fail',
                              reason=error.code if isinstance(error,InputError) else 'process-or-io-error',message=str(error))
            finally:
                record['seconds']=round(time.monotonic()-started,3)
                (run/'logs'/(name+'.stdout')).write_bytes(stdout);(run/'logs'/(name+'.stderr')).write_bytes(stderr)
                record.update(stdoutSha256=digest(run/'logs'/(name+'.stdout')),stderrSha256=digest(run/'logs'/(name+'.stderr')))
                result['phases'].append(record);write_json(run/(name+'.receipt.json'),record)
            if record['status']!='pass':break
        if len(result['phases'])==3 and all(phase['status']=='pass' for phase in result['phases']) and result['fixtureReceipts']['status']=='verified':
            result['status']='synthetic-control-pass' if context['inputKind']=='synthetic-control' else 'focused-producer-pass'
    except (InputError,OSError,ValueError,KeyError,TypeError) as error:
        result['issues'].append({'code':error.code if isinstance(error,InputError) else 'execution-error',
                                 'status':'blocked' if isinstance(error,InputError) and error.blocked else 'fail','message':str(error)})
    finally:
        write_json(run/'results.json',result)
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--inputs',required=True);parser.add_argument('--out',required=True);options=parser.parse_args()
    try:
        source=absolute_path(options.inputs);initial=digest(source);inputs=read_json(source);result=execute(inputs,options.out,(source,initial))
        print(json.dumps({'status':result['status'],'productGatePassed':False,'output':options.out}))
        raise SystemExit(3 if result['status'] in ('synthetic-control-pass','focused-producer-pass') else 1)
    except InputError as error:
        print(json.dumps({'code':error.code,'message':str(error),'productGatePassed':False}),file=sys.stderr)
        raise SystemExit(3 if error.blocked else 2)
    except (OSError,ValueError,KeyError,TypeError) as error:
        print(json.dumps({'code':'invalid-execution-input','message':str(error),'productGatePassed':False}),file=sys.stderr)
        raise SystemExit(2)
