#ifndef PLLIST_H
#define PLLIST_H

#define PLL_CAPACITY 32
#define PLL_PATH_MAX 127
#define PLL_STORAGE_BYTES 4096
#define PLL_NONE 0xFFFFU

typedef struct PLLModel {
    char path[PLL_CAPACITY][PLL_PATH_MAX + 1];
    unsigned short count;
    unsigned short selected;
    unsigned short top;
} PLLModel;

typedef enum PLLAddStatus {
    PLL_ADD_ADDED = 0,
    PLL_ADD_DUPLICATE = 1,
    PLL_ADD_FULL = 2,
    PLL_ADD_INVALID = 3
} PLLAddStatus;

typedef enum PLLResult {
    PLL_OK = 0,
    PLL_INVALID = 1
} PLLResult;

typedef enum PLLScrollCommand {
    PLL_SCROLL_UP = 1,
    PLL_SCROLL_DOWN = 2,
    PLL_SCROLL_HOME = 3,
    PLL_SCROLL_END = 4,
    PLL_SCROLL_PAGE_UP = 5,
    PLL_SCROLL_PAGE_DOWN = 6
} PLLScrollCommand;

void PLLInit(PLLModel *model);
PLLAddStatus PLLAdd(PLLModel *model, const char *path);
PLLResult PLLSelect(PLLModel *model, unsigned short index,
                    unsigned short visibleRows);
PLLResult PLLRemove(PLLModel *model, unsigned short index,
                    unsigned short visibleRows);
void PLLClear(PLLModel *model);
PLLResult PLLScroll(PLLModel *model, unsigned short command,
                    unsigned short visibleRows);
/* Returned near pointer remains valid until the model is mutated. */
const char *PLLGet(const PLLModel *model, unsigned short index);

#endif
