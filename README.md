# Godot Engine module for interacting with the Matrix protocol

This is a module for integrating a [Matrix](https://matrix.org) client into your Godot game. You can use it for building a chat service into your game, matchmaking/lobbies, or whatever else you can come up with!

Matrix is an open protocol and network which can be used for persistent communication (for example IM).

## Usage

There is a new node called `MatrixClient`. This represents a client. Set the homeserver address in the inspector or using `.set_hs_name(String name)`, then either log in using `.login(String username, String password)` _or_ set the access token in the inspector or using `.set_auth_token(String token)`. When you login you will get an access token for a session, and if you save this access token, you'll be able to continue using the same session at a later point. If the client is logged in (has an access token), calling `.logout()` will log out the current session.

Call `.start_listening()` to start receiving events from the Matrix server. The client will start long polling the homeserver for new events in the background. While this is happening, the `sync_token` in the `MatrixClient` will update periodically. If you don't want to receive the same old events a second time when you start up a new `MatrixClient`, save this sync token somewhere and restore it into the new `MatrixClient`. This will ensure you only receive the same event once.

You will receive all the events as signals (the signals will be emitted with the event as a `Dictionary` as the argument). `MatrixClient` currently has five different signals you can receive:

* `invited_to_room`: your account was invited to a room.

* `left_room`: you left a room (or were kicked/banned)

* `timeline_event`: received a timeline event in a room. These include received messages and state changes (e.g. someone changed the room name/topic, someone joined/left), and you'd usually want to list these events in a chat window in a typical IM app.

* `state_event`: the state changed in a room. You'll want to use this signal to update the UI (change the name, topic, user list etc.) if you're building an IM app, for example. _(Some events are emitted as both timeline events and state events. This is because they both belong on the timeline of messages, and also to update the overall UI. You should react to both signals accordingly.)_

* `ephemeral_event`: Events which are not persistent. This includes typing notifications, online/away/offline status, and read receipts.

The format of the `Dictionary` you receive in all of these signals is a Matrix `Event`, as specified in the [Matrix client-server API spec](https://matrix.org/docs/spec/client_server/r0.2.0.html).

You can call `.get_rooms()` on the `MatrixClient` to get all the rooms it is currently a member of. This is a dictionary mapping of room IDs to `MatrixRoom`s. If you only want to receive signals from one room instead of from all rooms, you can connect to `timeline_event`, `state_event`, and `ephemeral_event` on an individual `MatrixRoom` instead of on the `MatrixClient`. They behave the same way as described above.

You can leave a room using either the `.leave_room(String room_id)` method on the `MatrixClient`, or by calling `.leave_room()` on a `MatrixRoom`.

Sending events and state changes, and interacting with the server etc. is still very much a **work-in-progress**. Currently you can send text messages, by calling `.send_text_message(String text)` on a `MatrixRoom`.

There is an example Godot project in this repo, which contains a simple IM client to demonstrate how the module can be used (hardcoded to only chat in one room).

## Matrix basics

Here are the basics of the Matrix protocol, from the client's perspective.

A Matrix client connects to a _homeserver_. The client can then communicate with other clients on the same homeserver, and usually (if federation is not disabled) with clients on other homeservers.

All communication happens in _rooms_. Communication is done by sending _events_ to the room, which all _members_ of the room then receive. Clients can also update the _state_ of the room, for example change their display name and profile picture, change the name and topic of the room, or join, leave, invite, kick, or ban other members. Every member has a _power level_, which is a number which determines which of these things they are allowed to do in the room. Rooms are identified by their _room ID_ (which looks something like `!RtOeFVSpNUJHGLYYTh:vurpo.fi`), and can have _aliases_ which can also be used to identify a room (for example `#matrix:matrix.org`).

Everything that happens is an _event_ (various kinds exist). Events are represented as Dictionaries in this module. An event has a type, a room ID, an event ID, a sender, and some other fields. The full specification of what an event looks like can be found in the [Matrix client-server API spec](https://matrix.org/docs/spec/client_server/r0.2.0.html). The homeserver saves full history of all events that have happened.

## Credit

This module's design is heavily inspired by, and in some parts ported over from, the [Matrix Python SDK](https://github.com/matrix-org/matrix-python-sdk/). Thanks!
