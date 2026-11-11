#!/usr/bin/env python3
"""Reconstruct the observed naive stall calculation and require its regression to fail."""
import pathlib,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
source=(ROOT/'include/recorder.hpp').read_text()
start=source.index('        // Saturation prevents')
end=source.index('        double rpm',start)
legacy=source[:start]+'        bool stalled = !seen || core::elapsed(now,last_edge) >= 2000000;\n'+source[end:]
with tempfile.TemporaryDirectory() as temp:
    path=pathlib.Path(temp)
    (path/'recorder.hpp').write_text(legacy)
    subprocess.run(['c++','-std=c++17','-O2','-I'+str(path),'-I'+str(ROOT/'include'),str(ROOT/'tests/test.cpp'),'-o',str(path/'legacy')],check=True)
    result=subprocess.run([str(path/'legacy')],capture_output=True,text=True)
    assert result.returncode!=0 and 'out.stalled' in result.stderr, result.stdout+result.stderr
    print('EXPECTED LEGACY FAILURE:',result.stderr.strip())
    print('Current implementation passes the same long-idle regression in tools/test.sh.')
