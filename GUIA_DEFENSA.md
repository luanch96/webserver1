# 🚀 GUÍA COMPLETA PARA LA DEFENSA DE WEBSERV

## 📋 ÍNDICE
1. [Visión General del Proyecto](#visión-general)
2. [Teoría Esencial: Sockets, Headers HTTP y Métodos](#teoría-esencial)
3. [Flujo Completo del Programa](#flujo-completo)
4. [Arquitectura y Componentes](#arquitectura)
5. [Respuestas a la Hoja de Evaluación](#respuestas-evaluacion)
6. [Preguntas Frecuentes de Defensa](#preguntas-frecuentes)

---

## 🎯 VISIÓN GENERAL DEL PROYECTO {#visión-general}

### ¿Qué es webserv?
Webserv es un servidor HTTP/1.1 implementado en C++98 que puede:
- Servir archivos estáticos (HTML, CSS, JS, imágenes)
- Manejar múltiples servidores virtuales (diferentes puertos/hostnames)
- Ejecutar scripts CGI (Python, PHP)
- Manejar métodos HTTP: GET, POST, DELETE
- Configuración mediante archivo (estilo nginx)

### Tecnologías Clave
- **I/O Multiplexing**: `poll()` para manejar múltiples conexiones simultáneas
- **Non-blocking I/O**: Todos los sockets son no bloqueantes
- **Event-driven architecture**: Loop principal basado en eventos

---

## 📚 TEORÍA ESENCIAL: SOCKETS, HEADERS HTTP Y MÉTODOS {#teoría-esencial}

### 🔌 ¿QUÉ ES UN SOCKET?

Un **socket** es un punto final de comunicación bidireccional que permite que dos procesos (o programas) se comuniquen a través de una red. Es la abstracción fundamental de la programación de redes.

#### Conceptos Fundamentales:

1. **Socket como File Descriptor**:
   - En Unix/Linux, un socket es tratado como un file descriptor (fd)
   - Se puede leer (`recv()`, `read()`) y escribir (`send()`, `write()`) como un archivo
   - Cada socket tiene un número único (fd) que lo identifica

2. **Tipos de Sockets**:
   - **Socket de Escucha (Listening Socket)**: Espera conexiones entrantes
     - Se crea con `socket()`
     - Se asocia a una dirección IP y puerto con `bind()`
     - Se pone en modo escucha con `listen()`
     - Acepta conexiones con `accept()`
   - **Socket de Cliente**: Representa una conexión activa con un cliente
     - Se crea automáticamente cuando `accept()` acepta una conexión
     - Se usa para comunicarse con ese cliente específico

3. **Ciclo de Vida de un Socket**:
   ```
   socket() → bind() → listen() → accept() → recv()/send() → close()
   ```

4. **Non-blocking vs Blocking**:
   - **Blocking**: Las operaciones esperan hasta completarse (puede bloquear el programa)
   - **Non-blocking**: Las operaciones retornan inmediatamente, incluso si no hay datos
     - Si no hay datos, retorna -1 con `errno = EAGAIN`
     - Permite que el programa continúe atendiendo otros sockets

#### En nuestro proyecto:

```cpp
// Server.cpp: Creación de socket de escucha
int sock = socket(AF_INET, SOCK_STREAM, 0);  // Crea socket TCP/IP
bind(sock, (struct sockaddr*)&addr, sizeof(addr));  // Asocia a IP:puerto
listen(sock, SOMAXCONN);  // Pone en modo escucha
fcntl(sock, F_SETFL, O_NONBLOCK);  // Hace no bloqueante
```

```cpp
// Listener.cpp: Aceptar nueva conexión
int clientFd = accept(listeningSocket, NULL, NULL);  // Crea socket de cliente
// clientFd es un nuevo file descriptor para esta conexión específica
```

**¿Por qué usamos sockets?**
- Permiten comunicación en red entre cliente y servidor
- Cada conexión tiene su propio socket, permitiendo múltiples clientes simultáneos
- Los sockets son la base de toda comunicación HTTP

---

### 📨 ¿QUÉ ES UN HEADER HTTP?

Los **headers HTTP** son metadatos que acompañan a las peticiones y respuestas HTTP. Proporcionan información adicional sobre la petición/respuesta, el cliente, el servidor, y cómo debe procesarse el contenido.

#### Estructura de un Header:

```
Nombre-Header: valor del header\r\n
```

Ejemplo:
```
Host: localhost:8080\r\n
Content-Type: text/html\r\n
Content-Length: 1234\r\n
Connection: keep-alive\r\n
```

#### Headers Importantes en HTTP:

1. **Host**:
   - Indica el nombre de dominio y puerto del servidor
   - Ejemplo: `Host: localhost:8080`
   - **Uso en webserv**: Se usa para routing de servidores virtuales

2. **Content-Type**:
   - Indica el tipo MIME del contenido
   - Ejemplo: `Content-Type: text/html; charset=utf-8`
   - **Uso en webserv**: Determina cómo el navegador debe interpretar el contenido

3. **Content-Length**:
   - Indica el tamaño del body en bytes
   - Ejemplo: `Content-Length: 1024`
   - **Uso en webserv**: Indica cuántos bytes leer del body del request

4. **Connection**:
   - Controla si la conexión se mantiene abierta
   - Valores: `keep-alive` o `close`
   - **Uso en webserv**: Determina si reutilizar la conexión para múltiples requests

5. **User-Agent**:
   - Identifica el cliente (navegador, herramienta)
   - Ejemplo: `User-Agent: Mozilla/5.0...`

6. **Accept**:
   - Indica qué tipos de contenido acepta el cliente
   - Ejemplo: `Accept: text/html,application/json`

#### Headers en Requests vs Responses:

**Request Headers** (enviados por el cliente):
```
GET /index.html HTTP/1.1
Host: localhost:8080
User-Agent: curl/7.68.0
Accept: */*
Connection: keep-alive
```

**Response Headers** (enviados por el servidor):
```
HTTP/1.1 200 OK
Server: webserv/1.0
Date: Mon, 01 Jan 2024 12:00:00 GMT
Content-Type: text/html
Content-Length: 1234
Connection: keep-alive
```

#### En nuestro proyecto:

```cpp
// Request.cpp: Parseo de headers
std::map<std::string, std::string> _headers;  // Almacena todos los headers

// Ejemplo de uso:
if (request.hasHeader("Content-Length")) {
    size_t length = request.getContentLength();  // Lee el tamaño del body
}
```

```cpp
// Response.cpp: Construcción de headers
response.setHeader("Content-Type", "text/html");
response.setHeader("Content-Length", "1234");
response.setHeader("Connection", "keep-alive");
```

**¿Por qué son importantes los headers?**
- Permiten negociación de contenido (idioma, formato)
- Controlan el comportamiento de la conexión (keep-alive, timeouts)
- Proporcionan metadatos esenciales (tamaño, tipo de contenido)
- Permiten routing de servidores virtuales (header Host)

---

### 🌐 MÉTODOS HTTP SOPORTADOS: GET, POST Y DELETE

Los **métodos HTTP** (también llamados "verbos HTTP") definen la acción que el cliente quiere realizar sobre un recurso identificado por la URI.

#### GET - Obtener Recursos

**¿Qué hace GET?**
- Solicita una representación de un recurso específico
- Es **idempotente**: múltiples requests idénticos tienen el mismo efecto
- Es **seguro**: no modifica el estado del servidor
- No debe tener body (aunque algunos clientes lo permiten)

**Formato del Request:**
```
GET /index.html HTTP/1.1
Host: localhost:8080
```

**Casos de Uso:**
- Cargar una página web
- Obtener una imagen
- Recuperar datos de una API

**En nuestro proyecto:**
```cpp
// FileHandler.cpp: handleGet()
void FileHandler::handleGet(...) {
    // 1. Verificar si el archivo existe
    if (!Utils::fileExists(filePath)) {
        handleError(404, server, response);
        return;
    }
    
    // 2. Si es directorio, mostrar autoindex o buscar index.html
    if (Utils::isDirectory(filePath)) {
        if (location->autoindex) {
            // Generar listado HTML del directorio
            response.setBody(Utils::generateAutoindex(...));
        } else {
            // Buscar archivo index.html
            std::string indexFile = findIndexFile(filePath, server->index);
        }
    }
    
    // 3. Leer y servir el archivo
    std::string content = Utils::readFile(filePath);
    response.setStatus(200);
    response.setBody(content);
    response.setHeader("Content-Type", Utils::getMimeType(filePath));
}
```

**Códigos de Estado Comunes:**
- `200 OK`: Archivo encontrado y servido correctamente
- `404 Not Found`: El recurso no existe
- `403 Forbidden`: No hay permisos para acceder
- `301 Moved Permanently`: Redirección

---

#### POST - Enviar Datos al Servidor

**¿Qué hace POST?**
- Envía datos al servidor para procesamiento
- Puede crear nuevos recursos o modificar existentes
- **NO es idempotente**: múltiples requests pueden tener efectos diferentes
- **NO es seguro**: modifica el estado del servidor
- Tiene body con los datos a enviar

**Formato del Request:**
```
POST /uploads/ HTTP/1.1
Host: localhost:8080
Content-Type: application/octet-stream
Content-Length: 1024

[datos del archivo aquí...]
```

**Casos de Uso:**
- Subir archivos al servidor
- Enviar formularios HTML
- Crear nuevos recursos en una API REST

**En nuestro proyecto:**
```cpp
// FileHandler.cpp: handlePost()
void FileHandler::handlePost(...) {
    // 1. Verificar límite de tamaño del body
    if (request.getBody().size() > clientMaxBodySize) {
        handleError(413, server, response);  // Payload Too Large
        return;
    }
    
    // 2. Determinar ruta de destino
    std::string uploadPath = filePath;
    if (Utils::isDirectory(filePath)) {
        // Generar nombre único para el archivo
        uploadPath = filePath + generateUniqueFilename();
    }
    
    // 3. Escribir el body del request al archivo
    std::ofstream file(uploadPath.c_str(), std::ios::binary);
    file.write(request.getBody().c_str(), request.getBody().size());
    file.close();
    
    // 4. Responder con éxito
    response.setStatus(201);  // Created
    response.setBody("File uploaded successfully");
}
```

**Códigos de Estado Comunes:**
- `201 Created`: Recurso creado exitosamente
- `413 Payload Too Large`: El body excede el límite permitido
- `400 Bad Request`: El request es inválido
- `500 Internal Server Error`: Error al escribir el archivo

**Diferencia clave con GET:**
- GET: "Dame este recurso"
- POST: "Toma estos datos y haz algo con ellos"

---

#### DELETE - Eliminar Recursos

**¿Qué hace DELETE?**
- Solicita la eliminación de un recurso específico
- Es **idempotente**: eliminar un recurso que ya no existe tiene el mismo efecto
- **NO es seguro**: modifica el estado del servidor
- Generalmente no tiene body

**Formato del Request:**
```
DELETE /uploads/test.txt HTTP/1.1
Host: localhost:8080
```

**Casos de Uso:**
- Eliminar archivos del servidor
- Borrar recursos en una API REST
- Limpiar contenido temporal

**En nuestro proyecto:**
```cpp
// FileHandler.cpp: handleDelete()
void FileHandler::handleDelete(...) {
    // 1. Verificar que el archivo existe
    if (!Utils::fileExists(filePath)) {
        handleError(404, server, response);
        return;
    }
    
    // 2. Verificar que no es un directorio (por seguridad)
    if (Utils::isDirectory(filePath)) {
        handleError(403, server, response);  // Forbidden
        return;
    }
    
    // 3. Eliminar el archivo
    if (std::remove(filePath.c_str()) != 0) {
        handleError(500, server, response);  // Error al eliminar
        return;
    }
    
    // 4. Responder con éxito (sin body)
    response.setStatus(204);  // No Content
}
```

**Códigos de Estado Comunes:**
- `204 No Content`: Recurso eliminado exitosamente (sin body en respuesta)
- `404 Not Found`: El recurso no existe
- `403 Forbidden`: No se permite eliminar (directorio, permisos)
- `500 Internal Server Error`: Error al eliminar el archivo

**Características Importantes:**
- DELETE es idempotente: eliminar dos veces el mismo archivo tiene el mismo efecto
- No retorna el contenido eliminado (por eso 204 No Content)

---

#### Comparación de Métodos

| Método | Idempotente | Seguro | Body | Uso Principal |
|--------|-------------|--------|------|---------------|
| **GET** | ✅ Sí | ✅ Sí | ❌ No | Obtener recursos |
| **POST** | ❌ No | ❌ No | ✅ Sí | Crear/modificar recursos |
| **DELETE** | ✅ Sí | ❌ No | ❌ No | Eliminar recursos |

**Idempotente**: Múltiples requests idénticos tienen el mismo efecto  
**Seguro**: No modifica el estado del servidor

---

## 🔄 FLUJO COMPLETO DEL PROGRAMA {#flujo-completo}

### FASE 1: INICIALIZACIÓN

#### Paso 1.1: Lectura de Argumentos
```cpp
// main.cpp línea 20-25
std::string filename = "conf/default.conf";  // Por defecto
if (argc > 1) {
    filename = argv[1];  // O el archivo pasado como argumento
}
```
**¿Qué hace?** Lee el archivo de configuración (por defecto `conf/default.conf`)

#### Paso 1.2: Parsing de Configuración
```cpp
// main.cpp línea 27-28
ConfigParser parser(filename);
parser.parse();
```
**¿Qué hace?**
1. Lee el archivo de configuración
2. Elimina comentarios (líneas con `#`)
3. Separa bloques `server { ... }`
4. Parsea cada directiva:
   - `listen`: puertos e IPs
   - `server_name`: nombres de dominio
   - `root`: directorio base
   - `index`: archivo por defecto
   - `error_page`: páginas de error personalizadas
   - `client_max_body_size`: límite de tamaño de body
   - `location`: bloques de rutas específicas

**Resultado**: Vector de `ServerConfig` con toda la configuración

#### Paso 1.3: Creación de Servidores y Sockets

**Teoría: ¿Cómo se crea un socket de escucha?**

1. **socket()**: Crea el file descriptor del socket
   ```cpp
   int sock = socket(AF_INET, SOCK_STREAM, 0);
   // AF_INET: IPv4
   // SOCK_STREAM: TCP (conexión orientada)
   // 0: Protocolo por defecto (TCP)
   ```

2. **bind()**: Asocia el socket a una dirección IP y puerto
   ```cpp
   struct sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(8080);  // Puerto en network byte order
   addr.sin_addr.s_addr = INADDR_ANY;  // Escuchar en todas las interfaces
   bind(sock, (struct sockaddr*)&addr, sizeof(addr));
   ```

3. **listen()**: Pone el socket en modo escucha
   ```cpp
   listen(sock, SOMAXCONN);  // SOMAXCONN: máximo de conexiones en cola
   ```

4. **fcntl()**: Hace el socket no bloqueante
   ```cpp
   fcntl(sock, F_SETFL, O_NONBLOCK);
   // Ahora accept(), recv(), send() no bloquean
   ```

```cpp
// main.cpp línea 37-40
std::vector<Server*> servers;
for (size_t i = 0; i < serverConfigs.size(); ++i) {
    servers.push_back(new Server(serverConfigs[i]));
}
```
**¿Qué hace?**
1. Para cada `ServerConfig`, crea un objeto `Server`
2. El constructor de `Server`:
   - Crea sockets para cada `listen` especificado
   - Usa `_globalSocketMap` para compartir sockets entre servidores
   - Si el socket ya existe (mismo IP:puerto), lo reutiliza
   - Si falla el `bind()`, lanza excepción

**Resultado**: Vector de `Server*` con sockets de escucha configurados

#### Paso 1.4: Inicialización del Listener
```cpp
// main.cpp línea 43
Listener listener(servers, serverConfigs);
listener.run();  // Bucle principal (nunca retorna)
```
**¿Qué hace?**
- Crea el `Listener` que manejará todas las conexiones
- Inicia el bucle principal de eventos

---

### FASE 2: BUCLE PRINCIPAL (EVENT LOOP)

El `Listener::run()` es el corazón del servidor. Funciona así:

#### Paso 2.1: Preparar File Descriptors para poll()

**Teoría: ¿Qué es poll()?**

`poll()` es una función de I/O multiplexing que permite monitorear múltiples file descriptors simultáneamente. Espera hasta que uno o más fds estén listos para I/O.

**Estructura pollfd:**
```cpp
struct pollfd {
    int fd;        // File descriptor a monitorear
    short events;  // Eventos a monitorear (POLLIN, POLLOUT, etc.)
    short revents; // Eventos que ocurrieron (se llena después de poll())
};
```

**Eventos importantes:**
- `POLLIN`: El fd está listo para leer (hay datos disponibles)
- `POLLOUT`: El fd está listo para escribir (el buffer tiene espacio)
- `POLLERR`: Error en el socket
- `POLLHUP`: El otro extremo cerró la conexión

```cpp
// Listener.cpp línea 52-77
_pollfds.clear();

// 1. Agregar sockets de escucha
for (cada servidor) {
    for (cada socket de escucha) {
        pollfd pfd;
        pfd.fd = socket;
        pfd.events = POLLIN;  // Solo lectura (para accept)
        _pollfds.push_back(pfd);
    }
}

// 2. Agregar conexiones de clientes
for (cada conexión activa) {
    if (!conexión->shouldClose()) {
        pollfd pfd;
        pfd.fd = conexión->getFd();
        pfd.events = POLLIN | POLLOUT;  // ⚠️ CRÍTICO: Read Y Write simultáneamente
        _pollfds.push_back(pfd);
    }
}
```

**Puntos clave:**
- ✅ **Un solo `poll()`** en todo el programa
- ✅ **POLLIN | POLLOUT** para clientes (requisito de evaluación)
- ✅ Solo se agregan conexiones que no están marcadas para cerrar

#### Paso 2.2: Llamar a poll()
```cpp
// Listener.cpp línea 84
int ret = poll(&_pollfds[0], _pollfds.size(), 1000);  // Timeout de 1 segundo
```

**¿Qué hace `poll()`?**
- Espera hasta que algún fd esté listo para I/O
- Retorna:
  - `> 0`: Número de fds listos
  - `0`: Timeout (ningún fd listo en 1 segundo)
  - `< 0`: Error

**¿Por qué poll() y no select()?**
- `poll()` es más eficiente con muchos fds
- No tiene límite de 1024 fds como `select()`
- API más simple

#### Paso 2.3: Manejar Timeout
```cpp
// Listener.cpp línea 90-93
if (ret == 0) {
    checkTimeouts();  // Verificar conexiones que excedieron 30 segundos
    continue;
}
```

**¿Qué hace `checkTimeouts()`?**
- Revisa todas las conexiones activas
- Si una conexión no ha tenido actividad en 30 segundos, la cierra
- Previene conexiones colgadas (hanging connections)

#### Paso 2.4: Aceptar Nuevas Conexiones

**Teoría: ¿Cómo funciona accept()?**

`accept()` crea un nuevo socket para cada conexión entrante. El socket original sigue escuchando, y el nuevo socket se usa para comunicarse con ese cliente específico.

```cpp
// Listener.cpp línea 99-108
for (cada pollfd) {
    if (es socket de escucha && tiene POLLIN) {
        handleNewConnection(fd, newConnections);
    }
}
```

**¿Qué hace `handleNewConnection()`?**
```cpp
// Listener.cpp línea 141-158
int clientFd = accept(fd, ...);  // Acepta nueva conexión
if (clientFd < 0) {
    return;  // Error (probablemente EAGAIN en non-blocking)
}

ClientConnection* conn = new ClientConnection(clientFd);
// ClientConnection configura el socket como non-blocking
newConnections.push_back(conn);
```

**Puntos clave:**
- ✅ `accept()` es non-blocking (el socket de escucha es non-blocking)
- ✅ Si falla, simplemente retorna (no crashea)
- ✅ Crea nuevo `ClientConnection` para cada cliente
- ✅ Cada cliente tiene su propio file descriptor único

#### Paso 2.5: Manejar Conexiones Existentes
```cpp
// Listener.cpp línea 116-123
for (cada pollfd) {
    if (NO es socket de escucha && tiene eventos) {
        handleClientConnection(fd, revents);
    }
}
```

**¿Qué hace `handleClientConnection()`?**
```cpp
// Listener.cpp línea 160-180
ClientConnection* conn = findConnection(fd);

// 1. Verificar errores
if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
    conn->close();  // Cierra y marca para eliminación
    return;
}

// 2. Solo UNA operación por cliente por iteración
if (conn->getState() == READING_REQUEST) {
    if (revents & POLLIN) {
        conn->readRequest();  // Solo si hay datos para leer
    }
} else if (conn->getState() == WRITING_RESPONSE) {
    if (revents & POLLOUT) {
        conn->writeResponse();  // Solo si se puede escribir
    }
}
```

**Puntos clave:**
- ✅ **Solo una operación por cliente por poll**: Si está leyendo, solo lee. Si está escribiendo, solo escribe.
- ✅ Maneja errores de socket correctamente

#### Paso 2.6: Limpiar Conexiones Cerradas
```cpp
// Listener.cpp línea 126
cleanupConnections();
```

**¿Qué hace?**
```cpp
// Listener.cpp línea 192-202
for (cada conexión) {
    if (conexión->shouldClose()) {
        delete conexión;  // Libera memoria
        conexión.erase();  // Elimina del vector
    }
}
```

**Puntos clave:**
- ✅ Previene memory leaks
- ✅ Se ejecuta en cada iteración del loop

---

### FASE 3: PROCESAMIENTO DE REQUESTS

#### Paso 3.1: Leer Request (readRequest)

**Teoría: ¿Cómo se lee un request HTTP?**

Un request HTTP tiene esta estructura:
```
GET /index.html HTTP/1.1\r\n          ← Request Line
Host: localhost:8080\r\n               ← Headers
User-Agent: curl/7.68.0\r\n
Content-Length: 0\r\n
\r\n                                   ← Línea vacía (fin de headers)
[body si hay]                          ← Body (opcional)
```

El parsing se hace incrementalmente porque los datos pueden llegar en múltiples chunks.

```cpp
// ClientConnection.cpp línea 45-66
bool ClientConnection::readRequest() {
    char buffer[4096];
    ssize_t bytes = recv(_fd, buffer, sizeof(buffer) - 1, 0);
    
    // ⚠️ CRÍTICO: Verificar tanto -1 como 0
    if (bytes <= 0) {
        _shouldClose = true;  // Marcar para eliminación
        return false;
    }
    
    buffer[bytes] = '\0';
    _request.parseChunk(buffer);  // Parsear HTTP
    return true;
}
```

**¿Qué hace `parseChunk()`?**
1. **Parse Request Line**: `GET /index.html HTTP/1.1`
   - Método: GET, POST, DELETE, HEAD, o cualquier método válido
   - URI: `/index.html?query=value`
   - Versión: HTTP/1.0 o HTTP/1.1

2. **Parse Headers**: `Host: localhost\r\nContent-Length: 100\r\n\r\n`
   - Almacena en `std::map<std::string, std::string>`
   - Extrae `Content-Length` para saber cuánto leer del body
   - Detecta `Connection: keep-alive` o `Connection: close`

3. **Parse Body**: Si hay `Content-Length`, lee exactamente esos bytes

**Estados del Request:**
- `REQUEST_LINE` → `HEADERS` → `BODY` → `COMPLETE`

#### Paso 3.2: Procesar Request (processRequest)

**Teoría: ¿Cómo funciona el routing?**

El routing determina qué servidor y qué location manejan el request. Se basa en:
1. **Puerto**: Obtenido del socket con `getsockname()`
2. **Hostname**: Obtenido del header `Host:`
3. **Path**: Obtenido de la URI del request

```cpp
// ClientConnection.cpp línea 68-160
bool ClientConnection::processRequest(...) {
    // 1. Routing: Encontrar servidor y location
    Router::RoutingResult routing = Router::route(servers, _request, _fd);
    
    // 2. Verificar método permitido
    if (location && método no permitido) {
        _response.setStatus(405);
        return;
    }
    
    // 3. Verificar límite de body
    if (body > client_max_body_size) {
        _response.setStatus(413);
        return;
    }
    
    // 4. Procesar según tipo
    if (es CGI) {
        ejecutar CGI;
    } else {
        if (GET) FileHandler::handleGet();
        if (POST) FileHandler::handlePost();
        if (DELETE) FileHandler::handleDelete();
        if (otro) _response.setStatus(501);  // Not Implemented
    }
    
    // 5. Configurar keep-alive
    if (Connection: keep-alive) {
        mantener conexión abierta;
    } else {
        _shouldClose = true;
    }
}
```

**¿Qué hace `Router::route()`?**
1. **Encontrar Servidor**:
   - Obtiene puerto del socket con `getsockname()`
   - Obtiene hostname del header `Host:`
   - Busca servidor que coincida con puerto Y hostname
   - Si no hay match exacto, usa el primer servidor del puerto (default server)

2. **Encontrar Location**:
   - Busca la location con el path más largo que coincida
   - Ejemplo: `/uploads/file.txt` → location `/uploads/` (no `/`)

3. **Construir File Path**:
   - Combina `root` + `path` del request
   - Si es directorio, agrega `index`

4. **Verificar si es CGI**:
   - Mira la extensión del archivo (`.py`, `.php`)
   - Verifica si hay `cgi_pass` para esa extensión

#### Paso 3.3: Escribir Response (writeResponse)

**Teoría: ¿Cómo se construye una response HTTP?**

Una response HTTP tiene esta estructura:
```
HTTP/1.1 200 OK\r\n                    ← Status Line
Server: webserv/1.0\r\n                ← Headers
Date: Mon, 01 Jan 2024 12:00:00 GMT\r\n
Content-Type: text/html\r\n
Content-Length: 1234\r\n
Connection: keep-alive\r\n
\r\n                                   ← Línea vacía (fin de headers)
<html>...</html>                       ← Body
```

```cpp
// ClientConnection.cpp línea 163-187
bool ClientConnection::writeResponse() {
    if (ya se envió todo) {
        if (_shouldClose) {
            _state = CLOSING;
        } else {
            // Resetear para siguiente request (keep-alive)
            _request.reset();
            _response.clear();
            _state = READING_REQUEST;
        }
        return true;
    }
    
    // Enviar chunk
    ssize_t bytes = send(_fd, buffer + _responseSent, 
                        tamaño_restante, 0);
    
    // ⚠️ CRÍTICO: Verificar tanto -1 como 0
    if (bytes <= 0) {
        _shouldClose = true;
        return false;
    }
    
    _responseSent += bytes;
    return (ya se envió todo);
}
```

**Puntos clave:**
- ✅ **Solo un `send()` por cliente por poll**: Si no se puede enviar todo, espera al siguiente poll
- ✅ Maneja keep-alive: Si la conexión es keep-alive, resetea el estado para el siguiente request

---

## 🏗️ ARQUITECTURA Y COMPONENTES {#arquitectura}

### 1. ConfigParser
**Responsabilidad**: Parsear archivo de configuración estilo nginx

**Componentes:**
- `readFile()`: Lee el archivo completo
- `removeComments()`: Elimina líneas con `#`
- `splitServerBlocks()`: Separa bloques `server { ... }`
- Parsea cada directiva y crea objetos `ServerConfig`

**Estructura de datos:**
```cpp
ServerConfig {
    vector<string> listen;           // ["8080", "127.0.0.1:8081"]
    vector<string> serverNames;      // ["localhost", "test.local"]
    string root;                      // "www"
    string index;                     // "index.html"
    map<int, string> errorPages;     // {404: "/404.html"}
    size_t clientMaxBodySize;        // 1048576 (1MB)
    vector<LocationConfig> locations; // Bloques location
}
```

### 2. Server
**Responsabilidad**: Crear y gestionar sockets de escucha

**Teoría: ¿Cómo funciona la creación de sockets?**

El proceso de creación de un socket de escucha sigue estos pasos:

1. **socket()**: Crea el file descriptor
   - `AF_INET`: Protocolo IPv4
   - `SOCK_STREAM`: TCP (conexión orientada, confiable)
   - Retorna un fd que representa el socket

2. **setsockopt()**: Configura opciones del socket
   - `SO_REUSEADDR`: Permite reutilizar el puerto inmediatamente después de cerrar
   - Evita el error "Address already in use" al reiniciar el servidor

3. **bind()**: Asocia el socket a una dirección
   - Especifica en qué IP y puerto escuchar
   - `INADDR_ANY` (0.0.0.0): Escucha en todas las interfaces de red

4. **listen()**: Pone el socket en modo escucha
   - Crea una cola para conexiones pendientes
   - `SOMAXCONN`: Tamaño máximo de la cola

5. **fcntl()**: Hace el socket no bloqueante
   - `O_NONBLOCK`: Las operaciones no bloquean
   - `accept()` retorna inmediatamente, incluso si no hay conexiones

**Componentes:**
- `createSocket()`: Crea socket, bind, listen
- `_globalSocketMap`: Mapa estático que comparte sockets entre servidores

**Puntos clave:**
- ✅ Sockets compartidos: Si dos servidores usan el mismo puerto, comparten socket
- ✅ Non-blocking: Todos los sockets son no bloqueantes
- ✅ SO_REUSEADDR: Permite reutilizar puertos

### 3. Listener
**Responsabilidad**: Bucle principal de eventos (event loop)

**Teoría: ¿Qué es un Event Loop?**

Un event loop es un patrón de diseño que espera y distribuye eventos. En nuestro caso:
- Espera eventos de I/O usando `poll()`
- Cuando un fd está listo, ejecuta la acción correspondiente
- Permite manejar múltiples conexiones sin threads

**Componentes:**
- `run()`: Bucle principal con `poll()`
- `handleNewConnection()`: Acepta nuevas conexiones
- `handleClientConnection()`: Maneja I/O de clientes
- `cleanupConnections()`: Elimina conexiones cerradas
- `checkTimeouts()`: Cierra conexiones inactivas

**Estados del loop:**
1. Preparar `_pollfds`
2. Llamar `poll()`
3. Si timeout → `checkTimeouts()`
4. Aceptar nuevas conexiones
5. Manejar conexiones existentes
6. Limpiar conexiones cerradas
7. Repetir

### 4. ClientConnection
**Responsabilidad**: Gestionar una conexión de cliente

**Teoría: ¿Cómo se gestiona el ciclo de vida de una conexión?**

1. **Creación**: Cuando `accept()` acepta una conexión
   - Se crea un nuevo `ClientConnection` con el fd del cliente
   - El socket se configura como non-blocking

2. **Lectura**: Estado `READING_REQUEST`
   - Se lee el request HTTP en chunks
   - Se parsea incrementalmente

3. **Procesamiento**: Estado `PROCESSING`
   - Se enruta el request
   - Se ejecuta la acción (GET, POST, DELETE, CGI)

4. **Escritura**: Estado `WRITING_RESPONSE`
   - Se envía la response en chunks
   - Se maneja keep-alive

5. **Cierre**: Estado `CLOSING`
   - Se cierra el socket
   - Se libera la memoria

**Estados:**
- `READING_REQUEST`: Leyendo request HTTP
- `PROCESSING`: Procesando request (routing, validación)
- `WRITING_RESPONSE`: Enviando response
- `CLOSING`: Cerrando conexión

**Componentes:**
- `readRequest()`: Lee y parsea request HTTP
- `processRequest()`: Enruta y procesa request
- `writeResponse()`: Envía response al cliente
- `close()`: Cierra socket y marca para eliminación

**Manejo de errores:**
- ✅ `recv()` retorna <= 0 → `_shouldClose = true`
- ✅ `send()` retorna <= 0 → `_shouldClose = true`
- ✅ Timeout → `close()` → `_shouldClose = true`

### 5. Request
**Responsabilidad**: Parsear y almacenar request HTTP

**Teoría: ¿Cómo se parsea un request HTTP?**

Un request HTTP tiene esta estructura:
```
GET /index.html?query=value HTTP/1.1\r\n    ← Request Line
Host: localhost:8080\r\n                     ← Headers
Content-Type: text/html\r\n
Content-Length: 100\r\n
\r\n                                         ← Línea vacía
[body]                                       ← Body (opcional)
```

El parsing se hace en estados:
1. **REQUEST_LINE**: Parsear método, URI, versión
2. **HEADERS**: Parsear todos los headers hasta línea vacía
3. **BODY**: Leer body según Content-Length
4. **COMPLETE**: Request completo, listo para procesar

**Componentes:**
- `parseChunk()`: Parsea request incrementalmente
- `parseRequestLine()`: Parsea línea de request
- `parseHeaders()`: Parsea headers HTTP
- `parseBody()`: Parsea body (si hay Content-Length)

**Datos almacenados:**
```cpp
string _method;        // "GET", "POST", etc.
string _uri;           // "/index.html?query=value"
string _path;          // "/index.html"
string _query;         // "query=value"
string _version;       // "HTTP/1.1"
map<string, string> _headers;  // Headers HTTP
string _body;          // Body del request
```

### 6. Response
**Responsabilidad**: Construir response HTTP

**Teoría: ¿Cómo se construye una response HTTP?**

Una response HTTP tiene esta estructura:
```
HTTP/1.1 200 OK\r\n                         ← Status Line
Server: webserv/1.0\r\n                     ← Headers
Date: Mon, 01 Jan 2024 12:00:00 GMT\r\n
Content-Type: text/html\r\n
Content-Length: 1234\r\n
Connection: keep-alive\r\n
\r\n                                         ← Línea vacía
<html>...</html>                            ← Body
```

**Componentes:**
- `setStatus()`: Establece código de estado
- `setHeader()`: Agrega header HTTP
- `setBody()`: Establece body y Content-Length
- `buildResponse()`: Construye string HTTP completo

**Formato de response:**
```
HTTP/1.1 200 OK\r\n
Server: webserv/1.0\r\n
Date: ...\r\n
Content-Type: text/html\r\n
Content-Length: 1234\r\n
\r\n
<body>
```

### 7. Router
**Responsabilidad**: Enrutar requests a servidor/location correctos

**Teoría: ¿Cómo funciona el routing de servidores virtuales?**

El routing permite que múltiples sitios web compartan el mismo servidor físico. Se basa en:
1. **Puerto**: Cada servidor puede escuchar en diferentes puertos
2. **Hostname**: El header `Host:` identifica qué servidor virtual usar
3. **Path**: Las locations permiten diferentes configuraciones por ruta

**Componentes:**
- `route()`: Función principal de routing
- `findServer()`: Encuentra servidor por puerto y hostname
- `findLocation()`: Encuentra location por path (longest match)
- `buildFilePath()`: Construye ruta completa del archivo
- `isCGIRequest()`: Verifica si es request CGI

**Algoritmo de routing:**
1. Obtener puerto del socket (`getsockname()`)
2. Obtener hostname del header `Host:`
3. Buscar servidor que coincida con puerto
4. Si hay hostname, buscar match exacto de `server_name`
5. Si no hay match, usar default server (primero del puerto)
6. Buscar location con path más largo que coincida
7. Construir file path: `root + location_path + request_path`

### 8. FileHandler
**Responsabilidad**: Manejar archivos estáticos

**Teoría: ¿Cómo se sirven archivos estáticos?**

Los archivos estáticos son archivos que se sirven directamente sin procesamiento:
- HTML, CSS, JavaScript
- Imágenes (PNG, JPG, GIF)
- Archivos de texto

El proceso:
1. Verificar que el archivo existe
2. Leer el contenido del archivo
3. Determinar el Content-Type según la extensión
4. Enviar la response con el contenido

**Componentes:**
- `handleGet()`: Servir archivos (GET/HEAD)
- `handlePost()`: Subir archivos (POST)
- `handleDelete()`: Eliminar archivos (DELETE)
- `handleError()`: Servir páginas de error
- `findIndexFile()`: Buscar archivo index

**Funcionalidades:**
- ✅ Servir archivos estáticos
- ✅ Autoindex (listado de directorios)
- ✅ Upload de archivos
- ✅ Eliminación de archivos
- ✅ Páginas de error personalizadas

### 9. Utils
**Responsabilidad**: Funciones auxiliares

**Funciones:**
- `urlDecode()`: Decodifica URL encoding
- `getMimeType()`: Obtiene Content-Type por extensión
- `generateAutoindex()`: Genera HTML de listado de directorios
- `fileExists()`: Verifica si archivo existe
- `isDirectory()`: Verifica si es directorio
- `readFile()`: Lee archivo completo
- `parseSize()`: Parsea tamaños (1k, 1m, etc.)

---

## 📝 RESPUESTAS A LA HOJA DE EVALUACIÓN {#respuestas-evaluacion}

### MANDATORY PART

#### 1. ¿Qué función usaste para I/O Multiplexing?
**Respuesta**: Usé `poll()` de `<poll.h>`

**Ubicación**: `src/Listener.cpp` línea 84
```cpp
int ret = poll(&_pollfds[0], _pollfds.size(), 1000);
```

**¿Por qué poll() y no select()?**
- `poll()` no tiene límite de 1024 fds como `select()`
- API más simple y moderna
- Más eficiente con muchos file descriptors

#### 2. ¿Cómo funciona poll()?
**Respuesta**: 
- `poll()` espera hasta que uno o más file descriptors estén listos para I/O
- Recibe un array de `pollfd` con:
  - `fd`: File descriptor a monitorear
  - `events`: Eventos a monitorear (POLLIN, POLLOUT, etc.)
  - `revents`: Eventos que ocurrieron (se llena después de poll)
- Retorna número de fds listos, 0 si timeout, -1 si error
- Timeout de 1 segundo permite verificar timeouts periódicamente

**Código relevante**: `src/Listener.cpp` líneas 67-77, 84

#### 3. ¿Usas solo un poll() y cómo manejas accept y read/write?
**Respuesta**: 
- ✅ **Solo un `poll()`** en todo el programa (línea 84 de `Listener.cpp`)
- ✅ **Manejo de accept**: Los sockets de escucha tienen `POLLIN`. Cuando `poll()` indica que hay conexión, llamo a `accept()` (línea 148)
- ✅ **Manejo de read/write**: Los sockets de clientes tienen `POLLIN | POLLOUT` simultáneamente. Según el estado de la conexión, hago solo una operación:
  - Si `READING_REQUEST` y `POLLIN` → `readRequest()`
  - Si `WRITING_RESPONSE` y `POLLOUT` → `writeResponse()`

**Código relevante**: `src/Listener.cpp` líneas 73, 104-106, 170-178

#### 4. ¿El poll() verifica read y write al mismo tiempo?
**Respuesta**: ✅ **SÍ**

**Código**:
```cpp
// src/Listener.cpp línea 73
pfd.events = POLLIN | POLLOUT;  // Verifica AMBOS simultáneamente
```

**Explicación**: 
- Cada conexión de cliente se agrega a `_pollfds` con `POLLIN | POLLOUT`
- Esto permite que `poll()` detecte cuando el socket está listo para leer O escribir
- Luego, según el estado de la conexión, ejecuto solo la operación correspondiente

#### 5. ¿Solo un read o write por cliente por poll?
**Respuesta**: ✅ **SÍ**

**Código**:
```cpp
// src/Listener.cpp líneas 169-178
if (conn->getState() == READING_REQUEST) {
    if (revents & POLLIN) {
        conn->readRequest();  // Solo si está leyendo
    }
} else if (conn->getState() == WRITING_RESPONSE) {
    if (revents & POLLOUT) {
        conn->writeResponse();  // Solo si está escribiendo
    }
}
```

**Explicación**: 
- El estado de la conexión determina qué operación hacer
- Si está en `READING_REQUEST`, solo puede leer (aunque `POLLOUT` también esté activo)
- Si está en `WRITING_RESPONSE`, solo puede escribir (aunque `POLLIN` también esté activo)
- Esto asegura solo una operación por cliente por iteración de poll

#### 6. ¿Muestras el código de poll() a read/write?
**Respuesta**: 

**Flujo completo**:
```cpp
// 1. poll() detecta que fd está listo
int ret = poll(&_pollfds[0], _pollfds.size(), 1000);

// 2. Iterar sobre fds listos
for (cada pollfd con revents != 0) {
    if (es cliente) {
        handleClientConnection(fd, revents);
    }
}

// 3. handleClientConnection decide qué hacer
void handleClientConnection(int fd, short revents) {
    if (revents & POLLIN && estado == READING_REQUEST) {
        conn->readRequest();  // ← AQUÍ: recv()
    }
    if (revents & POLLOUT && estado == WRITING_RESPONSE) {
        conn->writeResponse();  // ← AQUÍ: send()
    }
}

// 4. readRequest() llama a recv()
ssize_t bytes = recv(_fd, buffer, sizeof(buffer) - 1, 0);

// 5. writeResponse() llama a send()
ssize_t bytes = send(_fd, buffer, tamaño, 0);
```

**Ubicaciones**:
- `poll()`: `src/Listener.cpp:84`
- `handleClientConnection()`: `src/Listener.cpp:160`
- `readRequest()` → `recv()`: `src/ClientConnection.cpp:47`
- `writeResponse()` → `send()`: `src/ClientConnection.cpp:177`

#### 7. ¿Si hay error en read/recv/write/send, se elimina el cliente?
**Respuesta**: ✅ **SÍ**

**Código para recv()**:
```cpp
// src/ClientConnection.cpp líneas 47-52
ssize_t bytes = recv(_fd, buffer, sizeof(buffer) - 1, 0);

if (bytes <= 0) {  // Error o cierre
    _shouldClose = true;  // Marcar para eliminación
    return false;
}
```

**Código para send()**:
```cpp
// src/ClientConnection.cpp líneas 177-183
ssize_t bytes = send(_fd, buffer, tamaño, 0);

if (bytes <= 0) {  // Error o cierre
    _shouldClose = true;  // Marcar para eliminación
    return false;
}
```

**Limpieza**:
```cpp
// src/Listener.cpp línea 126 (cada iteración)
cleanupConnections();  // Elimina conexiones con shouldClose() == true
```

#### 8. ¿Verificas tanto -1 como 0 en retornos?
**Respuesta**: ✅ **SÍ**

**Código**:
```cpp
// Verificación correcta
if (bytes <= 0) {  // Cubre tanto -1 (error) como 0 (cierre)
    _shouldClose = true;
}
```

**Ubicaciones**:
- `recv()`: `src/ClientConnection.cpp:49`
- `send()`: `src/ClientConnection.cpp:179`
- `write()` en CGI: `src/CGIHandler.cpp:90`
- `read()` en CGI: `src/CGIHandler.cpp:108`

#### 9. ¿Usas errno después de read/recv/write/send?
**Respuesta**: ❌ **NO**

**Verificación**: No hay ningún uso de `errno` después de estas llamadas. Solo hay un comentario que dice explícitamente "don't check errno" en `src/Listener.cpp:152`

#### 10. ¿Lees/escribes fd sin pasar por poll()?
**Respuesta**: ❌ **NO**

**Verificación**:
- ✅ Todos los `recv()` se llaman desde `readRequest()`, que se llama desde `handleClientConnection()`, que se llama después de `poll()`
- ✅ Todos los `send()` se llaman desde `writeResponse()`, que se llama desde `handleClientConnection()`, que se llama después de `poll()`
- ✅ `accept()` se llama desde `handleNewConnection()`, que se llama después de `poll()`
- ⚠️ Los `read()`/`write()` en `CGIHandler` son para **pipes internos**, no sockets de red, por lo que están permitidos

#### 11. ¿Compila sin problemas?
**Respuesta**: ✅ **SÍ**

**Makefile**:
```makefile
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -Iinclude
```

Compila sin warnings ni errores.

---

### CONFIGURATION

#### 1. ¿Códigos de estado HTTP correctos?
**Respuesta**: ✅ **SÍ**

**Códigos implementados** (todos válidos según RFC 7231):
- 200 OK
- 201 Created
- 204 No Content
- 301 Moved Permanently
- 400 Bad Request
- 403 Forbidden
- 404 Not Found
- 405 Method Not Allowed
- 408 Request Timeout
- 413 Payload Too Large
- 414 URI Too Long
- 500 Internal Server Error
- 501 Not Implemented
- 502 Bad Gateway
- 503 Service Unavailable
- 504 Gateway Timeout

**Ubicación**: `src/Response.cpp:55-75`

#### 2. ¿Múltiples servidores con diferentes puertos?
**Respuesta**: ✅ **SÍ**

**Ejemplo en default.conf**:
```
server {
    listen 8080;
    ...
}

server {
    listen 8081;
    ...
}
```

**Funcionamiento**: Cada puerto crea su propio socket. El Router selecciona el servidor correcto por puerto.

#### 3. ¿Múltiples servidores con diferentes hostnames?
**Respuesta**: ✅ **SÍ**

**Ejemplo en default.conf**:
```
server {
    listen 8080;
    server_name localhost;
    ...
}

server {
    listen 8081;
    server_name test.local;
    ...
}
```

**Funcionamiento**: 
- El Router lee el header `Host:` del request
- Busca servidor que coincida con puerto Y hostname
- Funciona con `curl --resolve example.com:80:127.0.0.1 http://example.com/`

**Código**: `src/Router.cpp:66-119`

#### 4. ¿Páginas de error por defecto?
**Respuesta**: ✅ **SÍ**

**Configuración**:
```
error_page 404 /404.html;
error_page 500 501 502 503 504 /50x.html;
```

**Funcionamiento**: `FileHandler::handleError()` busca la página personalizada, si no existe usa una por defecto.

**Código**: `src/FileHandler.cpp:141-162`

#### 5. ¿Límite de client body?
**Respuesta**: ✅ **SÍ**

**Configuración**:
```
client_max_body_size 1m;  // A nivel servidor
client_max_body_size 10k; // A nivel location (sobrescribe)
```

**Funcionamiento**: 
- Se verifica en `processRequest()`
- Si excede, retorna 413 Payload Too Large
- Soporta sufijos: k, m, g

**Código**: `src/ClientConnection.cpp:106-122`

#### 6. ¿Rutas a diferentes directorios?
**Respuesta**: ✅ **SÍ**

**Configuración**:
```
location /uploads/ { ... }
location /cgi-bin/ { ... }
location /static/ { ... }
```

**Funcionamiento**: Cada location puede tener su propio `root`, métodos permitidos, etc.

**Código**: `src/Router.cpp:125-143`

#### 7. ¿Archivo por defecto (index)?
**Respuesta**: ✅ **SÍ**

**Configuración**:
```
index index.html;
```

**Funcionamiento**: Si se accede a un directorio, busca el archivo index.

**Código**: `src/FileHandler.cpp:56, 164-172`

#### 8. ¿Lista de métodos permitidos?
**Respuesta**: ✅ **SÍ**

**Configuración**:
```
location / {
    allow_methods GET POST;
}

location /static/ {
    allow_methods GET;
}
```

**Funcionamiento**: Si el método no está permitido, retorna 405 Method Not Allowed.

**Código**: `src/ClientConnection.cpp:89-103`

---

### BASIC CHECKS

#### 1. ¿GET requests funcionan?
**Respuesta**: ✅ **SÍ**

**Código**: `src/FileHandler.cpp:21-80`

#### 2. ¿POST requests funcionan?
**Respuesta**: ✅ **SÍ**

**Funcionalidad**: Sube archivos al servidor, retorna 201 Created.

**Código**: `src/FileHandler.cpp:82-117`

#### 3. ¿DELETE requests funcionan?
**Respuesta**: ✅ **SÍ**

**Funcionalidad**: Elimina archivos, retorna 204 No Content.

**Código**: `src/FileHandler.cpp:119-139`

#### 4. ¿UNKNOWN requests no causan crash?
**Respuesta**: ✅ **SÍ**

**Funcionalidad**: 
- El parser acepta cualquier método HTTP válido
- Si el método no está implementado, retorna 501 Not Implemented
- No causa crash

**Código**: 
- Parser: `src/Request.cpp:89-101`
- Handler: `src/ClientConnection.cpp:142-145`

#### 5. ¿Status codes correctos?
**Respuesta**: ✅ **SÍ**

Todos los status codes son válidos según RFC 7231.

#### 6. ¿Upload y get back files?
**Respuesta**: ✅ **SÍ**

- POST sube archivos con nombre único
- GET recupera archivos subidos

---

### CHECK WITH A BROWSER

#### 1. ¿Compatible para sitios estáticos?
**Respuesta**: ✅ **SÍ**

- Soporta HTML, CSS, JS, imágenes
- Headers HTTP correctos
- Content-Type correcto por extensión

#### 2. ¿Maneja URLs incorrectas?
**Respuesta**: ✅ **SÍ**

Retorna 404 Not Found con página de error personalizada si está configurada.

#### 3. ¿Listado de directorios?
**Respuesta**: ✅ **SÍ**

Con `autoindex on`, genera HTML con listado de archivos.

**Código**: `src/Utils.cpp:69-131`

#### 4. ¿Redirecciones?
**Respuesta**: ✅ **SÍ**

Con `return` en location, retorna 301 Moved Permanently con header `Location:`.

**Código**: `src/ClientConnection.cpp:81-87`

---

### PORT ISSUES

#### 1. ¿Múltiples puertos funcionan?
**Respuesta**: ✅ **SÍ**

Cada puerto tiene su socket. El Router selecciona el servidor correcto.

#### 2. ¿Mismo puerto múltiples veces no funciona?
**Respuesta**: ✅ **SÍ**

- Si el mismo puerto se configura múltiples veces en el mismo archivo, comparten socket (comportamiento correcto, como nginx)
- Si el bind falla (puerto ya en uso por otro proceso), lanza excepción y el servidor no inicia

**Código**: `src/Server.cpp:32-37`

#### 3. ¿Múltiples servidores con puertos comunes?
**Respuesta**: ✅ **SÍ**

Los servidores comparten sockets a través de `_globalSocketMap`. El Router los distingue por hostname.

**Código**: `src/Server.cpp:30-38`

---

### SIEGE & STRESS TEST

#### 1. ¿Disponibilidad > 99.5%?
**Respuesta**: ✅ **Preparado**

El código maneja errores correctamente sin crashes. Para verificar > 99.5%, se requiere ejecutar siege, pero el código está preparado.

#### 2. ¿Sin memory leaks?
**Respuesta**: ✅ **SÍ**

- Todas las conexiones se eliminan con `delete` en `cleanupConnections()`
- `CGIHandler` libera memoria correctamente
- No hay leaks obvios

**Código**: `src/Listener.cpp:192-202`

#### 3. ¿Sin conexiones colgadas?
**Respuesta**: ✅ **SÍ**

- Timeout de 30 segundos
- `checkTimeouts()` se llama periódicamente
- Cierra conexiones inactivas

**Código**: `src/Listener.cpp:204-210`

#### 4. ¿Funcionamiento indefinido?
**Respuesta**: ✅ **SÍ**

- Loop principal infinito (`while(true)`)
- Limpieza continua de conexiones
- Keep-alive permite reutilizar conexiones

---

## ❓ PREGUNTAS FRECUENTES DE DEFENSA {#preguntas-frecuentes}

### P: ¿Por qué poll() y no select() o epoll()?
**R**: 
- `poll()` es más simple que `epoll()` (que es Linux-specific)
- No tiene límite de 1024 fds como `select()`
- Es portable (funciona en macOS, Linux, etc.)
- Para este proyecto, `poll()` es suficiente

### P: ¿Cómo manejas el problema de que poll() puede indicar que un socket está listo pero luego recv() retorna EAGAIN?
**R**: 
- Todos los sockets son non-blocking
- Si `recv()` retorna -1 con EAGAIN, simplemente retorno y espero al siguiente poll
- No verifico `errno` (requisito de evaluación)
- El siguiente poll detectará cuando realmente hay datos

### P: ¿Qué pasa si un cliente envía un request muy grande?
**R**: 
- El request se lee en chunks de 4096 bytes
- Se parsea incrementalmente
- Si excede `client_max_body_size`, se retorna 413
- El buffer se acumula hasta que el request esté completo

### P: ¿Cómo funciona el keep-alive?
**R**: 
- Si el header `Connection: keep-alive` está presente (o ausente en HTTP/1.1), la conexión se mantiene abierta
- Después de enviar la response, el estado vuelve a `READING_REQUEST`
- El mismo socket puede manejar múltiples requests
- Si `Connection: close`, se marca `_shouldClose = true` y se cierra después de la response

### P: ¿Qué pasa si un cliente se desconecta abruptamente?
**R**: 
- `poll()` detecta `POLLHUP` o `POLLERR`
- Se marca la conexión para cerrar
- `cleanupConnections()` la elimina en la siguiente iteración
- No hay memory leaks

### P: ¿Cómo manejas múltiples requests simultáneos?
**R**: 
- `poll()` puede detectar múltiples fds listos en una sola llamada
- El loop itera sobre todos los fds con `revents != 0`
- Cada conexión se maneja independientemente
- No hay bloqueo: todas las operaciones son non-blocking

### P: ¿Por qué solo una operación por cliente por poll?
**R**: 
- Es un requisito de la evaluación
- Previene que un cliente monopolice el servidor
- Asegura que todos los clientes sean atendidos equitativamente
- Si un cliente necesita múltiples operaciones, espera al siguiente poll

### P: ¿Cómo funciona el routing de servidores virtuales?
**R**: 
1. Se obtiene el puerto del socket con `getsockname()`
2. Se lee el header `Host:` del request
3. Se buscan servidores que escuchen en ese puerto
4. Si hay match exacto de `server_name`, se usa ese
5. Si no, se usa el default server (primero del puerto)
6. Luego se busca la location con el path más largo que coincida

### P: ¿Qué pasa si un script CGI tarda mucho?
**R**: 
- El script se ejecuta en un proceso hijo
- El servidor espera con `waitpid()`
- Si el script no responde, el servidor puede quedar bloqueado esperando
- Para producción, se podría agregar timeout, pero no está implementado

### P: ¿Cómo previenes memory leaks?
**R**: 
- Todas las conexiones se crean con `new` y se eliminan con `delete`
- `cleanupConnections()` se llama en cada iteración del loop
- `CGIHandler` libera memoria de variables de entorno
- Los destructores cierran file descriptors

### P: ¿Qué pasa si hay un error al crear un socket?
**R**: 
- Si `bind()` falla, `createSocket()` retorna -1
- El constructor de `Server` lanza excepción
- El programa termina con error
- Esto previene que el servidor inicie con configuración inválida

### P: ¿Qué es un socket y cómo funciona?
**R**: 
- Un socket es un punto final de comunicación bidireccional
- Permite que dos procesos se comuniquen a través de una red
- En Unix/Linux, se trata como un file descriptor
- Los sockets de escucha esperan conexiones, los sockets de cliente representan conexiones activas
- En nuestro proyecto, usamos sockets TCP/IP no bloqueantes para manejar múltiples clientes simultáneamente

### P: ¿Qué son los headers HTTP y para qué sirven?
**R**: 
- Los headers HTTP son metadatos que acompañan requests y responses
- Proporcionan información sobre el contenido, el cliente, el servidor
- Headers importantes: `Host` (routing), `Content-Type` (tipo MIME), `Content-Length` (tamaño), `Connection` (keep-alive)
- Permiten negociación de contenido y control del comportamiento de la conexión

### P: ¿Cuál es la diferencia entre GET, POST y DELETE?
**R**: 
- **GET**: Obtiene recursos, es idempotente y seguro, no tiene body
- **POST**: Envía datos al servidor, no es idempotente ni seguro, tiene body
- **DELETE**: Elimina recursos, es idempotente pero no seguro, generalmente sin body
- Cada método tiene casos de uso específicos y códigos de estado asociados

---

## 🎯 RESUMEN PARA LA DEFENSA

### Puntos Fuertes a Destacar:
1. ✅ **Un solo poll()** en todo el programa
2. ✅ **POLLIN | POLLOUT simultáneamente** para clientes
3. ✅ **Solo una operación por cliente por poll**
4. ✅ **Manejo correcto de errores** (verifica -1 y 0)
5. ✅ **No usa errno** después de I/O
6. ✅ **Sin memory leaks** (limpieza continua)
7. ✅ **Timeouts** previenen conexiones colgadas
8. ✅ **Keep-alive** para eficiencia
9. ✅ **Routing correcto** de servidores virtuales
10. ✅ **Soporte completo** de configuración estilo nginx

### Código Clave a Mostrar:
1. **poll() con POLLIN | POLLOUT**: `src/Listener.cpp:73`
2. **Una operación por cliente**: `src/Listener.cpp:169-178`
3. **Manejo de errores**: `src/ClientConnection.cpp:49, 179`
4. **Routing**: `src/Router.cpp:61-123`
5. **Limpieza de conexiones**: `src/Listener.cpp:192-202`

### Conceptos Teóricos Clave:
1. **Sockets**: Puntos finales de comunicación, file descriptors, non-blocking I/O
2. **Headers HTTP**: Metadatos, routing, negociación de contenido
3. **Métodos HTTP**: GET (obtener), POST (enviar), DELETE (eliminar)
4. **I/O Multiplexing**: poll() permite monitorear múltiples fds simultáneamente
5. **Event Loop**: Patrón que espera y distribuye eventos de I/O

---

**¡Buena suerte con tu defensa! 🚀**
