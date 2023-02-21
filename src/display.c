#include <display.h>

CONTAINER *current_container = NULL;

CONTAINER *ready_start_container = NULL;
CONTAINER *ready_end_container = NULL;

int create_screen() {


}

CONTAINER *remove_screen () {
    // TODO...
}

void attach_proc_to_screen(TASK_LL *proc, int container_id) {
    // Add to proc list of container
}

void remove_proc_from_screen(TASK_LL *proc, int container_id) {

}