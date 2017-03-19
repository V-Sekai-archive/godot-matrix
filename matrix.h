#ifndef MATRIX_H
#define MATRIX_H

#include "scene/main/node.h"

class MatrixClient : public Node {
  GDCLASS(MatrixClient,Node);

  String hs_name;

protected:
  static void _bind_methods();

public:
  String get_hs_name() const;
  String get_versions() const;

  MatrixClient();
};

#endif
  
