#include <stdio.h>
#include <stdlib.h>
#include "TPLAYPOL.H"
static unsigned checks;
#define TEST(x) do { ++checks; if(!(x)) { printf("FAIL line %u\n",__LINE__); return 1; } } while(0)
int main(void)
{
    unsigned i;
    static const char *const names[]={"HPXY.DRV","XRSYNC.DRV","TANDY88.DRV",
        "TNDY160.DRV","TNDY3204.DRV","TNDY6402.DRV","TNDY6404.DRV",
        "TR51616.DRV","TR53216.DRV","TR53204.DRV","TR56402.DRV",
        "TR56404.DRV"};
    TEST(TPDisplay("CGA.DRV")==0);TEST(TPDisplay("cga.drv")==0);
    for(i=0;i<sizeof(names)/sizeof(names[0]);++i)TEST(TPDisplay(names[i])==1);
    TEST(TPDisplay("xrsync.drv")==1);
    TEST(TPDisplay("XRSYNC2.DRV")==-1);
    TEST(TPDisplay("XRSYNC.DR_")==-1);
    TEST(TPDisplay("hpxy.drv")==1);TEST(TPDisplay("HPXY.DR_")==-1);
    TEST(TPDisplay("TXTMODE.DRV")==-1);TEST(TPDisplay("VGA.DRV")==-1);
    TEST(TPDisplay("")==-1);TEST(TPDisplay("C:\\HPXY.DRV")==-1);
    TEST(TPDisplay("OTHER.DRV")==-1);TEST(TPDisplay("HPXY.DRV ")==-1);
    TEST(TPBoard(0x21,0xff));TEST(!TPBoard(0x21,0xfd));
    TEST(!TPBoard(0,0xff));TEST(!TPBoard(0x21,0xfe));
    TEST(TPRomIRQ(0xf000,0xfea5));TEST(TPRomIRQ(0xfc00,0));
    TEST(TPRomIRQ(0xffff,0xf));TEST(!TPRomIRQ(0xffff,0x10));
    TEST(!TPRomIRQ(0xf000,0xbfff));TEST(!TPRomIRQ(0x9000,0));
    TEST(!TPBootstrapLength(255));TEST(TPBootstrapLength(256));
    TEST(TPBootstrapLength(7879));TEST(TPBootstrapLength(8398));
    TEST(TPBootstrapLength(65280));TEST(!TPBootstrapLength(65281));
    TEST(!TPBootstrapLength(-1));TEST(TPReserveStatus(0));
    TEST(TPReserveStatus(0x300));TEST(!TPReserveStatus(1));
    TEST(!TPReserveStatus(0x301));TEST(!TPReserveStatus(-1));
    TEST(!TPReserveStatus(0x100));TEST(!TPReserveStatus(0x200));
    printf("PASS %u policy checks\n",checks);return 0;
}
