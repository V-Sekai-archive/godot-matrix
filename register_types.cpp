#include "register_types.h"
#include "class_db.h"
#include "matrix.h"
#include "matrixroom.h"
#include "matrixuser.h"

void register_matrix_types() {
  ClassDB::register_class<MatrixClient>();
  ClassDB::register_class<MatrixRoom>();
  ClassDB::register_class<MatrixUser>();
}

void unregister_matrix_types() {
}
