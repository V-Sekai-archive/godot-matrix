#ifndef MATRIXUSER_H
#define MATRIXUSER_H

#include "core/reference.h"

#include "matrix.h"

class MatrixClient;

class MatrixUser : public Reference {
  friend class MatrixClient;
  GDCLASS(MatrixUser, Reference);

  MatrixClient *client;

  String user_id;
  String display_name;
  String avatar_url;

protected:
  static void _bind_methods();

public:

  String get_display_name(bool sync=false);
  String get_friendly_name(bool sync=false);

  Error set_display_name(String display_name);

  String get_avatar_url(bool sync=false);
  Error set_avatar_url(String mxcurl);

  MatrixUser();
  void init(MatrixClient *client, String user_id);

};
#endif
