#ifndef ENVIRON_H
#define ENVIRON_H

#include "node.h"

/* Environ format:

env handle
+---+
|   |
+---+
  |
  |
  v
top frame    (parent frame)
+---+---+    +---+---+
|   | ------>|   | -----> ...
+-|-+---+    +-|-+---+
  |            v
  |            ...
  v
entry        (next entry)
+---+---+    +---+---+
|   | ------>|   | -----> ...
+-|-+---+    +-|-+---+
  |            |
  v            v
keyval         ...
+---+---+
|   |   |
+-|-+-|-+
  |   +-----+
  |         |
  v         v
 key       val
+-------+  +-------+
|symbol |  |   ?   |
+-------+  +-------+
*/

void environ_pushframe(node_t *env_handle);
void environ_popframe(node_t *env_handle);
void environ_add(node_t *env_handle, node_t *key, node_t *val);

bool environ_keyval_frame(node_t *env_handle, node_t *key, node_t **keyval);
bool environ_keyval(node_t *env_handle, node_t *key, node_t **keyval);
bool environ_lookup(node_t *env_handle, node_t *key, node_t **val);

void environ_print(node_t *top_frame);

#endif
