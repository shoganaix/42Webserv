# 🐳 Desarrollo en Ubuntu con Docker

Si no tienes Ubuntu instalado, puedes usar Docker para ejecutar el proyecto en un entorno **idéntico a producción (Ubuntu)**.

---

## ⚙️ 1. Construir la imagen

Desde la carpeta donde está el `Dockerfile`:

```bash
docker build -t cpp-dev .
docker build --platform linux/amd64 -t cpp-dev .
```

👉 Crea una imagen:

* 📦 `cpp-dev`
* 🐧 Basada en Ubuntu (definido en el Dockerfile)

---

## 🔨 2. Crear y arrancar el contenedor

```bash
docker run -dit \
  --platform linux/amd64 \
  --name cpp-dev \
  -v $(pwd):/app \
  -w /app \
  -p 8080:8080 \
  cpp-dev bash
```

👉 Esto:

* monta tu código en `/app`
* expone el puerto **8080** del contenedor al host
* permite acceder al servidor desde el navegador


> ! 🌐 Acceso desde el navegador
> Una vez el servidor esté corriendo dentro del contenedor:
> 👉 Abre en tu navegador:
> ```
> http://localhost:8080
> ```


## 💻 3. Acceder al contenedor

```bash
docker exec -it cpp-dev bash
```

👉 Puedes abrir múltiples terminales

---

## ▶️ 4. Compilar y ejecutar el proyecto

Dentro del contenedor:

```bash
make
./webserv
./run_tests.sh
```

👉 Puedes:
* compilar
* levantar el servidor
* ejecutar el tester
* usar el script que guarda logs
* etc


> 🧠 Notas
>
> * ✔️ Todo se ejecuta dentro del contenedor
> * ✔️ Editas código en tu máquina → se refleja automáticamente
> * ✔️ Usa `cpp-dev` en lugar de container_id

---

## Notas: ⚠️ Puertos

Si cambias el puerto en tu servidor:

```bash
-p <host_port>:<container_port>
```

Ejemplo:

```bash
-p 3000:8080
```

👉 accederías desde:

```
http://localhost:3000
```

---

## 🧹 Comandos útiles

### Ver contenedor

```bash
docker ps
```

### Parar

```bash
docker stop cpp-dev
```

### Eliminar

```bash
docker rm cpp-dev
```

### Reentrar

```bash
docker exec -it cpp-dev bash
```
