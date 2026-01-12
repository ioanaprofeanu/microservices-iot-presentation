# Smart Factory Monitoring System
## Microservices Proof of Concept - Event-Driven Architecture

### 📋 Descriere Sistem

Sistem de monitorizare fabrică inteligentă implementat cu arhitectură de tip microservicii, folosind comunicare asincronă prin MQTT (Event-Driven Architecture).

**Componente:**
- 🔌 **MQTT Broker** (Eclipse Mosquitto) - Hub central de comunicare
- 🗄️ **PostgreSQL Database** - Stocare persistentă
- 🌡️ **Temperature Sensor** (C++) - Publică date de temperatură (20-80°C)
- 📳 **Vibration Sensor** (C++) - Publică date de vibrație (1000-5000 Hz)
- 📊 **Aggregator Service** (C++) - Colectează și stochează toate datele

### 🛠️ Stack Tehnologic

- **Limbaj:** C++17
- **Protocol:** MQTT (Paho MQTT C++)
- **Format Date:** JSON (nlohmann/json)
- **Database:** PostgreSQL (libpqxx)
- **Orchestrare:** Docker Compose

### 📁 Structura Proiectului

```
prezentare-microservicii/
├── docker-compose.yml          # Orchestrare containere
├── mosquitto/
│   └── mosquitto.conf          # Configurare MQTT broker
├── db/
│   └── init.sql                # Schema bazei de date
├── sensor-temp/
│   ├── Dockerfile
│   ├── CMakeLists.txt
│   └── main.cpp                # Senzor temperatură
├── sensor-vib/
│   ├── Dockerfile
│   ├── CMakeLists.txt
│   └── main.cpp                # Senzor vibrație
└── aggregator/
    ├── Dockerfile
    ├── CMakeLists.txt
    └── main.cpp                # Serviciu agregare
```

### 🚀 Rulare Aplicație

#### Prerequisite
- Docker Desktop instalat
- Docker Compose v2.0+

#### Comenzi

```bash
# 1. Build și pornire toate serviciile
docker-compose up --build

# 2. Rulare în background
docker-compose up -d --build

# 3. Vizualizare log-uri
docker-compose logs -f

# 4. Vizualizare log-uri pentru un serviciu specific
docker-compose logs -f aggregator

# 5. Oprire servicii
docker-compose down

# 6. Oprire și ștergere volume-uri (resetare DB)
docker-compose down -v
```

### 📊 Verificare Funcționare

#### 1. Verifică că toate serviciile rulează:
```bash
docker-compose ps
```

Expected output - toate serviciile ar trebui să fie "Up":
```
NAME                    STATUS
mqtt-broker             Up (healthy)
factory-db              Up (healthy)
sensor-temperature      Up
sensor-vibration        Up
aggregator-service      Up
```

#### 2. Monitorizare log-uri în timp real:

**Senzor Temperatură:**
```bash
docker-compose logs -f sensor-temperature
```
Output așteptat:
```
[Temperature Sensor] Published: {"type":"temperature","value":45.23}
[Temperature Sensor] Published: {"type":"temperature","value":67.89}
```

**Senzor Vibrație:**
```bash
docker-compose logs -f sensor-vibration
```
Output așteptat:
```
[Vibration Sensor] Published: {"type":"vibration","value":3456}
[Vibration Sensor] Published: {"type":"vibration","value":2134}
```

**Aggregator (cel mai important):**
```bash
docker-compose logs -f aggregator
```
Output așteptat:
```
[Aggregator] Received from topic "factory/sensors/temperature": {type: temperature, value: 45.23} -> Saved to DB ✓
[Aggregator] Received from topic "factory/sensors/vibration": {type: vibration, value: 3456} -> Saved to DB ✓
```

#### 3. Verificare date în PostgreSQL:

**Conectare la baza de date:**
```bash
docker exec -it factory-db psql -U factory_user -d factory_monitoring
```

**Interogare date:**
```sql
-- Afișează toate înregistrările
SELECT * FROM sensor_readings ORDER BY timestamp DESC LIMIT 20;

-- Statistici per tip de senzor
SELECT 
    sensor_type, 
    COUNT(*) as total_readings,
    AVG(value) as avg_value,
    MIN(value) as min_value,
    MAX(value) as max_value
FROM sensor_readings 
WHERE sensor_type != 'system'
GROUP BY sensor_type;

-- Ieșire din psql
\q
```

#### 4. Testare MQTT direct (opțional):

**Subscribe la toate topicurile:**
```bash
docker exec -it mqtt-broker mosquitto_sub -t "factory/sensors/#" -v
```

### 🏗️ Arhitectura Event-Driven

```
┌─────────────────┐         ┌─────────────────┐
│  Sensor Temp    │────────▶│                 │
│  (Publisher)    │         │   MQTT Broker   │
└─────────────────┘         │   (Mosquitto)   │
                            │                 │
┌─────────────────┐         │                 │         ┌─────────────────┐
│  Sensor Vib     │────────▶│                 │────────▶│   Aggregator    │
│  (Publisher)    │         │                 │         │  (Subscriber)   │
└─────────────────┘         └─────────────────┘         └────────┬────────┘
                                                                  │
                                                                  ▼
                                                         ┌─────────────────┐
                                                         │   PostgreSQL    │
                                                         │    Database     │
                                                         └─────────────────┘
```

**Flow de date:**
1. Senzorii publică date la intervale regulate (2s și 3s)
2. MQTT Broker distribuie mesajele către toți subscriber-ii
3. Aggregator primește mesajele și le salvează în DB
4. Toate comunicările sunt asincrone (fire-and-forget)

### 🔧 Configurări și Parametri

**Environment Variables (configurabile în docker-compose.yml):**

| Variabilă | Default | Descriere |
|-----------|---------|-----------|
| MQTT_BROKER | mqtt-broker | Hostname MQTT |
| MQTT_PORT | 1883 | Port MQTT |
| PUBLISH_INTERVAL | 2/3 | Interval publicare (secunde) |
| DB_HOST | database | Hostname PostgreSQL |
| DB_NAME | factory_monitoring | Nume DB |
| DB_USER | factory_user | User DB |
| DB_PASSWORD | factory_pass | Parolă DB |

### 🐛 Troubleshooting

**1. Serviciile nu pornesc:**
```bash
# Verifică log-urile
docker-compose logs

# Reconstruiește fără cache
docker-compose build --no-cache
docker-compose up
```

**2. Erori de conectare MQTT:**
- Verifică că mqtt-broker este "healthy": `docker-compose ps`
- Așteptă ~30s după pornire pentru stabilizare
- Verifică log-urile: `docker-compose logs mqtt-broker`

**3. Erori de conectare Database:**
- Verifică că database este "healthy": `docker-compose ps`
- Verifică credențialele în docker-compose.yml
- Reset database: `docker-compose down -v && docker-compose up --build`

**4. Nu apar date în DB:**
```bash
# Verifică dacă senzorii publică
docker-compose logs sensor-temperature sensor-vibration

# Verifică dacă aggregator primește mesaje
docker-compose logs aggregator

# Verifică manual datele
docker exec -it factory-db psql -U factory_user -d factory_monitoring -c "SELECT COUNT(*) FROM sensor_readings;"
```

### 📈 Extensii Posibile

1. **Dashboard Web:** Adaugă frontend (React/Vue) pentru vizualizare live
2. **Alert System:** Notificări când valori depășesc praguri
3. **Time-Series Analytics:** Integrare cu InfluxDB/Grafana
4. **Load Balancing:** Multiple aggregator instances
5. **Message Persistence:** MQTT QoS 2 + retained messages
6. **Authentication:** TLS/SSL pentru MQTT + autentificare clienți

### 🎓 Prezentare Master - Puncte Cheie

✅ **Arhitectură Microservicii** - Servicii independente, loosely coupled
✅ **Event-Driven Pattern** - Comunicare asincronă prin MQTT
✅ **Containerizare** - Docker multi-stage builds
✅ **Scalabilitate** - Ușor de adăugat noi senzori
✅ **Resilience** - Auto-reconnect, health checks
✅ **Production-Ready** - Logging, error handling, retry logic

### 📝 Licență

Acest proiect este creat în scop educațional pentru prezentare de master.

---

