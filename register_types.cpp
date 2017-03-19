#include "register_types.h"
#include "class_db.h"
#include "matrix.h"

void register_matrix_types() {
  ClassDB::register_class<MatrixClient>();
}

void unregister_matrix_types() {
}
