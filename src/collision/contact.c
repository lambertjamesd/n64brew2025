#include "contact.h"

bool contacts_are_touching(contact_t* first, entity_id other_object) {
    for (contact_t* curr = first; curr; curr = curr->next) {
        if (curr->other_object == other_object) {
            return true;
        }
    }
    
    return false;
}