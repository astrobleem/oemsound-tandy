#include <stdio.h>
static int checks,failures;
void Check(char *name,int yes){checks++;if(!yes){printf("FAIL %s\n",name);failures++;}}
extern void CoreTests(void);
int main(void){CoreTests();printf("FINAL failures=%d checks=%d\n",failures,checks);return failures!=0;}
