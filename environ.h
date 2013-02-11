#ifndef ENVIRON_H
#define ENVIRON_H

#include "node.h"

/* Environ format:

environ      parent environ
+---+---+    +---+---+
|   | ------>|   | -----> ...
+-|-+---+    +---+---+
  |
  v
child        next child
+---+---+    +---+---+
|   | ------>|   | -----> ...
+-|-+---+    +---+---+
  |
  v
keyval
+---+---+
|   |   |
+-|-+-|-+
  |   +-----+
  |         |
  v         v
key        val
+-------+  +-------+
|symbol |  |   ?   |
+-------+  +-------+
*/

/* returned value has refcount = 1 */
node_t *environ_push(node_t *environ);

/* environ will leak if you don't release it */
node_t *environ_add(node_t *environ, node_t *key, node_t *val);

/* retain of *value required */
bool environ_lookup(node_t *environ, node_t *key, node_t **value);

void environ_print(node_t *environ);

#endif
