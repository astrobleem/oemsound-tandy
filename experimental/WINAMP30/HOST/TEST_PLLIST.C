#include <stdio.h>
#include <string.h>
#include "PLLIST.H"

static int checks;
static int failures;

#define C(x) do { checks++; if (!(x)) { \
    printf("FAIL line %d: %s\n", __LINE__, #x); failures++; } } while (0)

static void add_numbered(PLLModel *model, unsigned short count)
{
    unsigned short i;
    char path[PLL_PATH_MAX + 1];
    for (i = 0; i < count; ++i) {
        sprintf(path, "C:\\MUSIC\\TRACK%02u.MOD", i);
        C(PLLAdd(model, path) == PLL_ADD_ADDED);
    }
}

static void test_storage_and_add(void)
{
    PLLModel model;
    unsigned short i;
    char path[PLL_PATH_MAX + 2];

    PLLInit(&model);
    C(sizeof(model.path) == PLL_STORAGE_BYTES);
    C(PLL_CAPACITY == 32 && PLL_PATH_MAX == 127);
    C(model.count == 0 && model.selected == PLL_NONE && model.top == 0);
    add_numbered(&model, 5);
    C(model.count == 5 && model.selected == 0);
    C(strcmp(PLLGet(&model, 4), "C:\\MUSIC\\TRACK04.MOD") == 0);

    PLLClear(&model);
    for (i = 0; i < PLL_CAPACITY; ++i) {
        sprintf(path, "D:\\AUDIO\\ITEM%02u.MOD", i);
        C(PLLAdd(&model, path) == PLL_ADD_ADDED);
    }
    C(model.count == 32);
    C(PLLAdd(&model, "D:\\AUDIO\\EXTRA.MOD") == PLL_ADD_FULL);
    C(model.count == 32);
    C(PLLAdd(&model, "d:\\audio\\item00.mod") == PLL_ADD_DUPLICATE);
    C(model.count == 32);

    PLLClear(&model);
    memset(path, 'X', sizeof(path));
    path[PLL_PATH_MAX] = '\0';
    C(PLLAdd(&model, path) == PLL_ADD_ADDED);
    C(strlen(PLLGet(&model, 0)) == PLL_PATH_MAX);
    path[PLL_PATH_MAX] = 'X';
    path[PLL_PATH_MAX + 1] = '\0';
    C(PLLAdd(&model, path) == PLL_ADD_INVALID);
    C(PLLAdd(&model, "") == PLL_ADD_INVALID);
    C(PLLAdd(&model, 0) == PLL_ADD_INVALID);
    C(model.count == 1);
}

static void test_duplicate_and_remove(void)
{
    PLLModel model;
    char active[PLL_PATH_MAX + 1];

    PLLInit(&model);
    C(PLLAdd(&model, "C:\\Mix\\Set1.MOD") == PLL_ADD_ADDED);
    C(PLLAdd(&model, "C:\\Mix\\Set2.MOD") == PLL_ADD_ADDED);
    C(PLLAdd(&model, "C:\\Mix\\Set3.MOD") == PLL_ADD_ADDED);
    C(PLLAdd(&model, "c:\\mIX\\sET1.mod") == PLL_ADD_DUPLICATE);
    C(model.count == 3);

    C(PLLSelect(&model, 2, 2) == PLL_OK);
    strcpy(active, PLLGet(&model, model.selected));
    C(PLLRemove(&model, 2, 2) == PLL_OK); /* Remove current last item. */
    C(strcmp(active, "C:\\Mix\\Set3.MOD") == 0);
    C(model.count == 2 && model.selected == 1);
    C(strcmp(PLLGet(&model, 1), "C:\\Mix\\Set2.MOD") == 0);

    C(PLLRemove(&model, 0, 2) == PLL_OK); /* Remove first, before selection. */
    C(model.count == 1 && model.selected == 0 && model.top == 0);
    C(strcmp(PLLGet(&model, 0), "C:\\Mix\\Set2.MOD") == 0);

    C(PLLAdd(&model, "C:\\Mix\\Set4.MOD") == PLL_ADD_ADDED);
    C(PLLAdd(&model, "C:\\Mix\\Set5.MOD") == PLL_ADD_ADDED);
    C(PLLSelect(&model, 1, 2) == PLL_OK);
    C(PLLRemove(&model, 1, 2) == PLL_OK); /* Remove current middle item. */
    C(model.count == 2 && model.selected == 1);
    C(strcmp(PLLGet(&model, 1), "C:\\Mix\\Set5.MOD") == 0);
    C(PLLRemove(&model, 1, 2) == PLL_OK); /* Remove last; select previous. */
    C(model.count == 1 && model.selected == 0);

    C(PLLRemove(&model, 0, 2) == PLL_OK);
    C(model.count == 0 && model.selected == PLL_NONE && model.top == 0);
    C(PLLGet(&model, 0) == 0);
    C(PLLRemove(&model, 0, 2) == PLL_INVALID);
}

static void test_scroll_and_selection(void)
{
    PLLModel model;
    unsigned short i;
    unsigned short previous;

    PLLInit(&model);
    add_numbered(&model, 12);
    C(PLLSelect(&model, 9, 4) == PLL_OK);
    C(model.selected == 9 && model.top == 6);
    C(model.selected >= model.top && model.selected < model.top + 4);
    C(PLLSelect(&model, 12, 4) == PLL_INVALID);
    C(model.selected == 9 && model.top == 6);

    C(PLLScroll(&model, PLL_SCROLL_HOME, 4) == PLL_OK);
    C(model.selected == 0 && model.top == 0);
    for (i = 0; i < 6; ++i)
        C(PLLScroll(&model, PLL_SCROLL_DOWN, 4) == PLL_OK);
    C(model.selected == 6 && model.top == 3);
    C(model.selected >= model.top && model.selected < model.top + 4);
    C(PLLScroll(&model, PLL_SCROLL_UP, 4) == PLL_OK);
    C(model.selected == 5 && model.top == 3);
    C(PLLScroll(&model, PLL_SCROLL_PAGE_DOWN, 4) == PLL_OK);
    C(model.selected == 9 && model.top == 6);
    C(PLLScroll(&model, PLL_SCROLL_PAGE_DOWN, 4) == PLL_OK);
    C(model.selected == 11 && model.top == 8);
    C(PLLScroll(&model, PLL_SCROLL_END, 4) == PLL_OK);
    C(model.selected == 11 && model.top == 8);
    C(PLLScroll(&model, PLL_SCROLL_DOWN, 4) == PLL_OK);
    C(model.selected == 11 && model.top == 8);
    C(PLLScroll(&model, PLL_SCROLL_UP, 4) == PLL_OK);
    C(model.selected == 10 && model.top == 8);
    C(PLLScroll(&model, PLL_SCROLL_END, 4) == PLL_OK);
    C(model.selected == 11 && model.top == 8);
    C(PLLScroll(&model, PLL_SCROLL_PAGE_UP, 4) == PLL_OK);
    C(model.selected == 7 && model.top == 7);
    C(PLLScroll(&model, PLL_SCROLL_HOME, 4) == PLL_OK);
    C(model.selected == 0 && model.top == 0);
    C(PLLScroll(&model, PLL_SCROLL_UP, 4) == PLL_OK);
    C(model.selected == 0 && model.top == 0);
    C(PLLScroll(&model, PLL_SCROLL_PAGE_UP, 4) == PLL_OK);
    C(model.selected == 0 && model.top == 0);

    previous = model.selected;
    C(PLLScroll(&model, 99, 4) == PLL_INVALID);
    C(model.selected == previous);
    C(PLLScroll(&model, PLL_SCROLL_DOWN, 0) == PLL_INVALID);
    C(PLLSelect(&model, 0, 0) == PLL_INVALID);

    C(PLLRemove(&model, 0, 4) == PLL_OK);
    C(model.count == 11 && model.selected == 0 && model.top == 0);
    C(model.selected >= model.top && model.selected < model.top + 4);
    PLLClear(&model);
    C(model.count == 0 && model.selected == PLL_NONE && model.top == 0);
    C(PLLScroll(&model, PLL_SCROLL_HOME, 4) == PLL_INVALID);
}

static void test_append_preserves_view(void)
{
    PLLModel model;
    PLLInit(&model);
    add_numbered(&model, 8);
    C(PLLSelect(&model, 5, 3) == PLL_OK);
    C(model.selected == 5 && model.top == 3);
    C(PLLAdd(&model, "C:\\MUSIC\\APPENDED.MOD") == PLL_ADD_ADDED);
    C(model.count == 9 && model.selected == 5 && model.top == 3);
}

int main(void)
{
    PLLModel model;
    test_storage_and_add();
    test_duplicate_and_remove();
    test_scroll_and_selection();
    test_append_preserves_view();
    C(PLLAdd(0, "X") == PLL_ADD_INVALID);
    PLLInit(0);
    PLLClear(0);
    printf("playlist model host tests: %d checks, %d failures\n",
           checks, failures);
    (void)model;
    return failures != 0;
}
