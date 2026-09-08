"""Run against a freshly built executable: python test_warmingup6.py path/to/program.exe"""
import pathlib
import subprocess
import sys
import tempfile
import unittest

EXECUTABLE = pathlib.Path(sys.argv.pop(1)).resolve()
PROJECT = pathlib.Path(__file__).resolve().parents[1]
BASE = 'v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\n'


def run_reports(inputs):
    with tempfile.TemporaryDirectory(prefix='warmingup6-') as directory:
        root = pathlib.Path(directory)
        for i, data in enumerate(inputs, 1):
            (root / f'data{i}.txt').write_text(data, encoding='utf-8-sig')
        reports = []
        for i in range(1, len(inputs) + 1):
            run = subprocess.run([str(EXECUTABLE)], cwd=root,
                                 input=f'data{i}.txt\nresult{i}.txt\n',
                                 encoding='utf-8', capture_output=True, timeout=10)
            output = root / f'result{i}.txt'
            data = inputs[i - 1]
            displayed = data + ('\n' if data and not data.endswith('\n') else '')
            assert '원본 데이터 시작 ----------\n' + displayed + '---------- 원본 데이터 끝' in run.stdout
            if run.returncode == 0:
                saved = output.read_text(encoding='utf-8-sig')
                assert run.stdout.split('저장된 결과:\n', 1)[1] == saved
                assert run.stdout.index('결과 저장 위치:') < run.stdout.index('Input file:')
                reports.append(saved)
            else:
                assert not output.exists(), 'Invalid input must not create a result file'
                assert '저장할 파일명' not in run.stdout
                reports.append(run.stdout)
        return reports


class ParserTests(unittest.TestCase):
    def test_error_example_files(self):
        expected = {
            4: ('Errors: 4', '세 개의 숫자', '허용되지 않는 문자', '정확히 세 개'),
            5: ('Errors: 6', '범위를 벗어났습니다', '잘못된 정점 데이터', '잘못된 텍스처 데이터'),
            6: ('Errors: 4', '중복됩니다', '같은 정점 인덱스', '동일한 정점 좌표', '일직선'),
        }
        for i, messages in expected.items():
            with self.subTest(file=i):
                report = self.report((PROJECT / f'data{i}.txt').read_text(encoding='utf-8-sig'))
                for message in messages:
                    self.assertIn(message, report)

    def report(self, data):
        return run_reports([data])[0]

    def test_interactive_paths_and_failures(self):
        with tempfile.TemporaryDirectory(prefix='warmingup6-') as directory:
            root = pathlib.Path(directory)
            source = root / '입력 데이터.txt'
            source.write_text(BASE + 'f 1 2 3', encoding='utf-8')
            def run(text):
                return subprocess.run([str(EXECUTABLE)], cwd=root, input=text,
                                      encoding='utf-8', capture_output=True, timeout=10)
            output = root / '저장 결과.txt'
            self.assertEqual(run(f'{source}\n{output}\n').returncode, 0)
            self.assertIn('No errors', output.read_text(encoding='utf-8-sig'))
            original = source.read_bytes()
            self.assertNotEqual(run(f'{source}\n{source}\n').returncode, 0)
            self.assertEqual(source.read_bytes(), original)
            for text in ('missing.txt\n', '\n', f'{source}\n\n',
                         f'{source}\nmissing/result.txt\n'):
                self.assertNotEqual(run(text).returncode, 0)

    def test_original_examples(self):
        inputs = [(PROJECT / f'data{i}.txt').read_text(encoding='utf-8-sig')
                  for i in range(1, 4)]
        for i, report in enumerate(run_reports(inputs), 1):
            self.assertEqual(report, (PROJECT / f'result{i}.txt').read_text(encoding='utf-8-sig'))

    def test_required_errors(self):
        invalid = [BASE + face for face in (
            'f 1 2', 'f 1 2 3 1', 'f 1 1 3', 'f 1 2 4',
            'f 1/1 2/2 3/4', 'f 0 2 3', 'f -1 2 3', 'f 1 2 a',
            'f 1/1 2 3', 'f 1//1 2/2 3/3')]
        invalid += [BASE + 'v 0 0 0\nf 1 2 4', 'v 2 0 0', 'vt 1.1 0']
        for data in invalid:
            with self.subTest(data=data):
                self.assertIn('Errors:', self.report(data))

    def test_strict_numbers(self):
        for data in ('v 0.0.5 0', 'v 0.0.5 0 0', 'v 0-1 0',
                     'vt 0.0.5', 'vt 0.0.5 0', 'v 1e 0 0'):
            with self.subTest(data=data):
                self.assertIn('Errors:', self.report(data))
        valid = '# comment\nv +0 .0 -0\nv 1e-1 0 0\nv 0 .1 0 # inline\nvt .5 1.\nf 1/1 2/1 3/1'
        self.assertIn('No errors', self.report(valid))

    def test_degenerate_triangles(self):
        for data in ('v -1 0 0\nv 0 0 0\nv 1 0 0\nf 1 2 3',
                     'v 0 0 0\nv .3 .3 .3\nv .6 .6 .6\nf 1 2 3',
                     'v 0 0 0\nv 1 0 0\nv .5 1e-14 0\nf 1 2 3'):
            report = self.report(data)
            self.assertIn('Errors:', report)
            self.assertNotIn('Face 1 (', report)
        tiny = 'v 0 0 0\nv 1e-100 0 0\nv 0 1e-100 0\nf 1 2 3'
        self.assertIn('No errors', self.report(tiny))

    def test_invalid_vertex_preserves_indices(self):
        for invalid in ('v 2 0 0', 'v bad 0 0', 'v 0.0.5 0'):
            data = 'v 0 0 0\n' + invalid + '\nv 0 1 0\nv 0 0 1\nf 1 2 3\nf 1 3 4'
            report = self.report(data)
            self.assertIn('Vertex count: 4', report)
            self.assertNotIn('Face 1 (', report)
            self.assertIn('Face 2 (1, 3, 4):\nvertex (0.000000, 0.000000, 0.000000) (0.000000, 1.000000, 0.000000) (0.000000, 0.000000, 1.000000)', report)

    def test_invalid_texture_preserves_indices(self):
        for invalid in ('vt 2 0', 'vt bad 0', 'vt 0.0.5'):
            data = 'v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\n' + invalid + '\nvt 1 0\nvt 0 1\nf 1/1 2/2 3/3\nf 1/1 2/3 3/4'
            report = self.report(data)
            self.assertIn('Texture count: 4', report)
            self.assertNotIn('Face 1 (', report)
            self.assertIn('Face 2 (1, 2, 3):', report)
            self.assertIn('texture (0.000000, 0.000000) (1.000000, 0.000000) (0.000000, 1.000000)', report)


if __name__ == '__main__':
    unittest.main()
