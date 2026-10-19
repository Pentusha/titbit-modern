"""Regression: largest X and Y providers form an impossible archive ordering."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from elf_comdat import select_comdats


class LargestComdatTest(unittest.TestCase):
    def test_independent_selection_with_cyclic_provider_order(self):
        with tempfile.TemporaryDirectory(prefix='titbit-comdat-') as folder:
            root=Path(folder)
            rows=[]
            for name,x,y,body in (
                ('a','11,12','21','movl table_X+4, %eax\naddl table_Y, %eax'),
                ('b','91','31,32','movl table_X, %eax\naddl table_Y+4, %eax')):
                source=root/(name+'.s');obj=root/(name+'.o')
                source.write_text(f'''
.section .rodata.x,"aG",@progbits,table_X,comdat
.globl table_X
.type table_X,@object
table_X: .long {x}
.size table_X, .-table_X
.section .rodata.y,"aG",@progbits,table_Y,comdat
.globl table_Y
.type table_Y,@object
table_Y: .long {y}
.size table_Y, .-table_Y
.text
.globl {name}
{name}:
{body}
ret
.section .note.GNU-stack,"",@progbits
''')
                subprocess.run(['as','--32',str(source),'-o',str(obj)],check=True)
                rows.append({'source':name,'object':str(obj),'comdatSelections':[
                    {'symbol':'table_X','selection':'largest'},
                    {'symbol':'table_Y','selection':'largest'}]})
            proofs=select_comdats(rows)
            self.assertEqual({p['symbol']:p['winner'] for p in proofs},{'table_X':'a','table_Y':'b'})
            main=root/'main.c'
            main.write_text('''#include <stdio.h>
extern int a(void),b(void);
int main(void){int x=a(),y=b();printf("largest COMDAT cycle: %d,%d\\n",x,y);return x!=43 || y!=43;}
''')
            subprocess.run(['ar','rcs',str(root/'game.a'),*[r['object'] for r in rows]],check=True)
            exe=root/'check'
            subprocess.run(['gcc','-m32','-no-pie',str(main),str(root/'game.a'),'-o',str(exe)],check=True)
            result=subprocess.run([str(exe)],capture_output=True,text=True,check=True)
            self.assertIn('43,43',result.stdout)


if __name__=='__main__':unittest.main()
