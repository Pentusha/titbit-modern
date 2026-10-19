extern "C" int align_zero();
extern "C" int align_one(int);
extern "C" int align_three(int,int,int);
extern "C" int align_six(int,int,int,int,int,int);
extern "C" int align_double(double,int);
extern "C" int __stdcall align_stdcall(int,int,int);
extern "C" int __thiscall align_thiscall(void*,int,int);
extern "C" int alignment_results[8];
struct Member {
 int pad;
 __declspec(noinline) int check(int n) {volatile char local[7];local[0]=char(n);return align_three(n,local[0],3);}
};
extern "C" int probe_alignment(int n) {
 volatile char local[11];local[0]=char(n);
 alignment_results[0]=align_zero();
 alignment_results[1]=align_one(local[0]);
 alignment_results[2]=align_three(1,2,3);
 alignment_results[3]=align_six(1,2,3,4,5,6);
 alignment_results[4]=align_double(42.0,7);
 alignment_results[5]=align_stdcall(1,2,3);
 Member member;
 alignment_results[6]=align_thiscall(&member,1,2);
 alignment_results[7]=member.check(n);
 int errors=0;
 for(int i=0;i<8;i++)if(alignment_results[i])++errors;
 return errors;
}
