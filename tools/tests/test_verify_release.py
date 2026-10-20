"""A manifest must not substitute for comparing the actual archived EXE."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

VERIFY = Path(__file__).resolve().parents[1]/'verify_release.py'


class ReleaseVerificationTests(unittest.TestCase):
    def verify_fixture(self, different_executable=False, different_metadata=False):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary); build=root/'build'; build.mkdir()
            payloads={'TitbitModern.exe':b'MZ benign game fixture',
                      'TitbitSettings.exe':b'MZ benign settings fixture'}
            digest=lambda value:hashlib.sha256(value).hexdigest()
            expected={name:digest(data) for name,data in payloads.items()}
            for name,data in payloads.items():(build/name).write_bytes(data)
            (build/'build-info.json').write_text('{}')
            if different_executable:payloads['TitbitModern.exe']=b'MZ different benign fixture'
            if different_metadata:expected['TitbitModern.exe']='0'*64
            payloads['package.json']=json.dumps({'version':'test','sourceCommit':'test',
                                               'executables':expected}).encode()
            payloads['SHA256SUMS.txt']=''.join(digest(data)+'  '+name+'\n'
                                               for name,data in payloads.items()).encode()
            archive=root/'release.zip'
            with zipfile.ZipFile(archive,'w') as output:
                for name,data in payloads.items():output.writestr('TitbitModern/'+name,data)
            result=subprocess.run([sys.executable,str(VERIFY),str(archive),'--build',str(build)],
                                  capture_output=True,text=True,encoding='utf-8')
            return result.returncode,result.stdout

    def test_matching_archive_and_build_pass(self):
        self.assertEqual(self.verify_fixture()[0],0)

    def test_changed_archive_with_consistent_checksums_is_rejected(self):
        code,output=self.verify_fixture(different_executable=True)
        self.assertEqual(code,1)
        self.assertIn('TitbitModern.exe: DIFFERENT',output)

    def test_incorrect_executable_metadata_is_rejected(self):
        self.assertEqual(self.verify_fixture(different_metadata=True)[0],1)


if __name__=='__main__':unittest.main()
