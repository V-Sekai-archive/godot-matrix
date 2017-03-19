#ifndef MATRIX_H
#define MATRIX_H

#include "scene/main/node.h"
#include "core/io/http_client.h"

class MatrixClient : public Node {
  GDCLASS(MatrixClient,Node);

  HTTPClient m_client;
  String hs_name;
  String auth_token;

  String request(String endpoint, String body, HTTPClient::Method method);

protected:
  static void _bind_methods();

public:
  String get_hs_name() const;
  void set_hs_name(String name);

  Error login(String username, String password);

  String get_versions();

  MatrixClient();
};

#endif
  
