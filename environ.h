#ifndef ENVIRON_H
#define ENVIRON_H

#include "node.h"

/* Environ format:

environ
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

/* *environ released and replaced with new environ */
void environ_add(node_t **environ, node_t *key, node_t *val);

/* *value and *keyvalue are retained before return */
bool environ_keyvalue(node_t *environ, node_t *key, node_t **keyvalue);
bool environ_lookup(node_t *environ, node_t *key, node_t **value);

void environ_print(node_t *environ);

#endif
