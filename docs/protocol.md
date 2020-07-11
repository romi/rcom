
TODO: check https://www.jsonrpc.org/specification


# Registry

The registry has two types of interactions with the other nodes:

* Request/response: Nodes can send requests to the registry, such as
  register/unregister. The registry will send a response to signal the
  success of the request and possible pass a return value.
* Events: Nodes get notified when nodes appear or dissappear. Events
  are sent only by the registry to all nodes in the network.


## Example

Request:

```json
{"request": "list"}
```

Response:

```json
{
  "response": "list-response",
  "success": true,
  "list": [
    {
      "name": "registry",
      "id": "ed8ae24b-fbfe-4320-ae2e-4115e95b9c6d",
      "type": "messagehub",
      "topic": "registry",
      "addr": "0.0.0.0:10101"
    },
    {
      "name": "configuration",
      "id": "289a0c98-6bed-4336-92b3-c04b806f36f1",
      "type": "service",
      "topic": "configuration",
      "addr": "0.0.0.0:47275"
    },
    {
      "name": "configuration",
      "id": "c4d06932-1552-4905-a6ed-f2c59350c0eb",
      "type": "messagelink",
      "topic": "logger",
      "addr": "0.0.0.0:0"
    }
  ]
}
```

## Entry object

The entry object describes a communication end-point in the
network.

The possible types of end-points are: datahub, datalink, messagehub,
messagelink, service, streamer, and streamerlink.

Hubs, streamers, and services are uniquely identified by their type
and topic. So there can be only one datahub called "encoders", or one
streamer called "camera". However, their can be many (client-)links
that are interested in the same topic.

The entry object is used in the several requests and events. It is a
JSON object with the following fields:


| Name  | Type   | Description  |
| ----- | ------ | ------------ |
| id    | String | The internal ID |
| name  | String | The user-given name |
| type  | String | The type of the entry (datahub, datalink, messagehub, ...). |
| topic | String | The topic of this entry |
| addr  | String | The IP address of the entry (useful only for hubs and streamers) |


## Requests and responses

### Request object

A request is a JSON-encoded object. It's main field is `request`. The
possible request values are:


### Request values

| Value          | Description | Response    | Event        |
| -------------- | -------- | -------------- | ------------ |
| register       | To register a new entry | register-response       | proxy-add    |
| unregister     | To unregister a new entry | unregister-response     | proxy-remove |
| update-address | To update the address of a entry | update-address-response | proxy-update-address |
| list           | To list all the available entries | list-response           | - |


### Response object

A JSON object with the following fields:

| Name     | Type    | Description  |
| -------- | ------- | ------------ |
| response | String  | One of the values from the table below |
| success  | Boolean | Whether the request was executed succesfully or not |
| message  | String  | An error message. Only informative when success is false. |
| list     | Array of entries | Used only by "list" requests |


The `response` field takes the same value as the original request:
`register`, `unregister`, `update-address`, or `list`.


## Events

Events are sent by the registry when the overlay network changes. Events are coded in JSON as follows.

### Event object

An event is a JSON object with the field `event` set to one of the
following values:

| Value                | Description |
| -------------------- | ----------- | 
| proxy-add            | A new entry was registered. |
| proxy-remove         |  |
| proxy-update-address |  |

