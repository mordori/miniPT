#include "utils.h"

static inline void print_editing(void);
static inline void print_general(void);

void printf_init(void) {
	printf("\n");
	printf("                  NAVIGATION                        \n");
	printf("|---------------------|----------------------------|\n");
	printf("|         Key         |           Action           |\n");
	printf("|---------------------|----------------------------|\n");
	printf("| RMB                 | Look                       |\n");
	printf("| ALT + LMB           | Orbit                      |\n");
	printf("| ALT + RMB           | Zoom                       |\n");
	printf("| ALT + MMB           | Pan                        |\n");
	printf("| F                   | Frame                      |\n");
	printf("|---------------------|----------------------------|\n");
	printf("\n");
	print_editing();
	print_general();
}

static inline void print_editing(void) {
	printf("                   EDITING                          \n");
	printf("|---------------------|----------------------------|\n");
	printf("|         Key         |           Action           |\n");
	printf("|---------------------|----------------------------|\n");
	printf("| TAB                 | Toggle Edit Mode           |\n");
	printf("| LMB                 | Select / Apply Edit        |\n");
	printf("| RMB                 | Cancel Edit                |\n");
	printf("| G                   | Translate                  |\n");
	printf("| R                   | Rotate                     |\n");
	printf("| S                   | Scale                      |\n");
	printf("| X / Y / Z           | Axis Constraint            |\n");
	printf("| SHIFT + X / Y / Z   | Planar Constraint          |\n");
	printf("| V                   | View Constraint            |\n");
	printf("| SHIFT + D           | Duplicate                  |\n");
	printf("| DEL                 | Delete                     |\n");
	printf("|---------------------|----------------------------|\n");
	printf("\n");
}

static inline void print_general(void) {
	printf("                   GENERAL                          \n");
	printf("|---------------------|----------------------------|\n");
	printf("|         Key         |           Action           |\n");
	printf("|---------------------|----------------------------|\n");
	printf("| ESC                 | Quit                       |\n");
	printf("|---------------------|----------------------------|\n");
	printf("\n");
}
