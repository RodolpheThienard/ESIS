
#include <Adafruit_BME680.h>
#include <Adafruit_Sensor.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

// Configuration WiFi
const char *ssid = "Wifi";                // Remplacez par votre nom WiFi
const char *password = "WifiRodolphe2.0"; // Remplacez par votre mot de passe

// Créer les objets
Adafruit_BME680 bme;
WebServer server (80);

// Variables pour stocker les dernières mesures
float temperature = 0;
float humidity = 0;
float pressure = 0;
float gasResistance = 0;
String qualiteAir = "";
int iaq = 0;

void
setup ()
{
  Serial.begin (115200);
  Serial.println ("\n\n=== STATION MÉTÉO ESP32 ===");

  // Initialiser le BME680
  Serial.println ("Initialisation BME680...");
  if (!bme.begin (0x76))
    {
      if (!bme.begin (0x77))
        {
          Serial.println ("❌ BME680 introuvable!");
          while (1)
            ;
        }
    }

  // Configuration du capteur
  bme.setTemperatureOversampling (BME680_OS_8X);
  bme.setHumidityOversampling (BME680_OS_2X);
  bme.setPressureOversampling (BME680_OS_4X);
  bme.setIIRFilterSize (BME680_FILTER_SIZE_3);
  bme.setGasHeater (320, 150);

  Serial.println ("✅ BME680 initialisé");
  Serial.println ("⏳ Préchauffage 10s...");
  delay (10000);

  // Connexion WiFi
  Serial.println ("\nConnexion WiFi...");
  WiFi.begin (ssid, password);

  int tentatives = 0;
  while (WiFi.status () != WL_CONNECTED && tentatives < 20)
    {
      delay (500);
      Serial.print (".");
      tentatives++;
    }

  if (WiFi.status () == WL_CONNECTED)
    {
      Serial.println ("\n✅ WiFi connecté!");
      Serial.print ("📡 Adresse IP: ");
      Serial.println (WiFi.localIP ());
    }
  else
    {
      Serial.println ("\n❌ Échec connexion WiFi");
    }

  // Configuration des routes du serveur web
  server.on ("/", handleRoot);
  server.on ("/data", handleData);
  server.on ("/style.css", handleCSS);

  server.begin ();
  Serial.println ("🌐 Serveur web démarré");
  Serial.println ("===========================\n");
}

void
loop ()
{
  server.handleClient ();

  // Lecture des capteurs toutes les 2 secondes
  static unsigned long derniereLecture = 0;
  if (millis () - derniereLecture > 10000)
    {
      derniereLecture = millis ();

      if (bme.performReading ())
        {
          temperature = bme.temperature;
          humidity = bme.humidity;
          pressure = bme.pressure / 100.0;
          gasResistance = bme.gas_resistance / 1000.0;
          qualiteAir = evaluerQualiteAir (gasResistance);
          iaq = calculerIAQ (gasResistance, humidity);

          // Affichage dans le moniteur série
          Serial.println ("--- Mesures ---");
          Serial.printf ("🌡️  %.1f°C | 💧 %.1f%% | 🔽 %.1f hPa | 🌫️  "
                         "%.2f KΩ | %s\n",
                         temperature, humidity, pressure, gasResistance,
                         qualiteAir.c_str ());
        }
    }
}

// Page principale HTML
void
handleRoot ()
{
  String html = "<!DOCTYPE html><html lang='fr'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, "
          "initial-scale=1.0'>";
  html += "<title>Station Météo ESP32</title>";
  html += "<link rel='stylesheet' href='/style.css'>";
  html += "</head><body>";

  html += "<div class='container'>";
  html += "<h1>🌤️ Station Météo ESP32</h1>";
  html += "<p class='subtitle'>Capteur BME680 - Mise à jour automatique</p>";

  html += "<div class='cards'>";

  // Carte Température
  html += "<div class='card'>";
  html += "<div class='icon'>🌡️</div>";
  html += "<div class='label'>Température</div>";
  html += "<div class='value' id='temp'>--</div>";
  html += "<div class='unit'>°C</div>";
  html += "</div>";

  // Carte Humidité
  html += "<div class='card'>";
  html += "<div class='icon'>💧</div>";
  html += "<div class='label'>Humidité</div>";
  html += "<div class='value' id='humidity'>--</div>";
  html += "<div class='unit'>%</div>";
  html += "</div>";

  // Carte Pression
  html += "<div class='card'>";
  html += "<div class='icon'>🔽</div>";
  html += "<div class='label'>Pression</div>";
  html += "<div class='value' id='pressure'>--</div>";
  html += "<div class='unit'>hPa</div>";
  html += "</div>";

  // Carte Gaz
  html += "<div class='card'>";
  html += "<div class='icon'>🌫️</div>";
  html += "<div class='label'>Résistance Gaz</div>";
  html += "<div class='value' id='gas'>--</div>";
  html += "<div class='unit'>KΩ</div>";
  html += "</div>";

  html += "</div>"; // fin cards

  // Qualité de l'air
  html += "<div class='air-quality'>";
  html += "<h2>Qualité de l'Air</h2>";
  html += "<div class='quality-badge' id='quality'>Chargement...</div>";
  html += "<div class='iaq'>Indice IAQ: <span id='iaq'>--</span></div>";
  html += "</div>";

  html += "<div class='footer'>Dernière mise à jour: <span "
          "id='time'>--</span></div>";
  html += "</div>"; // fin container

  // JavaScript pour mise à jour automatique
  html += "<script>";
  html += "function updateData() {";
  html += "  fetch('/data')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      document.getElementById('temp').textContent = "
          "data.temperature;";
  html += "      document.getElementById('humidity').textContent = "
          "data.humidity;";
  html += "      document.getElementById('pressure').textContent = "
          "data.pressure;";
  html += "      document.getElementById('gas').textContent = data.gas;";
  html += "      document.getElementById('quality').textContent = "
          "data.quality;";
  html += "      document.getElementById('iaq').textContent = data.iaq;";
  html += "      const now = new Date();";
  html += "      document.getElementById('time').textContent = "
          "now.toLocaleTimeString('fr-FR');";
  html += "    });";
  html += "}";
  html += "updateData();";
  html
      += "setInterval(updateData, 2000);"; // Mise à jour toutes les 2 secondes
  html += "</script>";

  html += "</body></html>";

  server.send (200, "text/html", html);
}

// Route pour les données JSON
void
handleData ()
{
  String json = "{";
  json += "\"temperature\":\"" + String (temperature, 1) + "\",";
  json += "\"humidity\":\"" + String (humidity, 1) + "\",";
  json += "\"pressure\":\"" + String (pressure, 1) + "\",";
  json += "\"gas\":\"" + String (gasResistance, 2) + "\",";
  json += "\"quality\":\"" + qualiteAir + "\",";
  json += "\"iaq\":\"" + String (iaq) + "\"";
  json += "}";

  server.send (200, "application/json", json);
}

// CSS embarqué
void
handleCSS ()
{
  String css = "* { margin: 0; padding: 0; box-sizing: border-box; }";
  css += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, "
         "sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 "
         "100%); min-height: 100vh; padding: 20px; }";
  css += ".container { max-width: 1200px; margin: 0 auto; }";
  css += "h1 { color: white; text-align: center; font-size: 2.5em; "
         "margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }";
  css += ".subtitle { color: rgba(255,255,255,0.9); text-align: center; "
         "margin-bottom: 40px; font-size: 1.1em; }";
  css += ".cards { display: grid; grid-template-columns: repeat(auto-fit, "
         "minmax(250px, 1fr)); gap: 20px; margin-bottom: 30px; }";
  css += ".card { background: white; border-radius: 15px; padding: 30px; "
         "text-align: center; box-shadow: 0 10px 30px rgba(0,0,0,0.2); "
         "transition: transform 0.3s; }";
  css += ".card:hover { transform: translateY(-5px); }";
  css += ".icon { font-size: 3em; margin-bottom: 10px; }";
  css += ".label { color: #666; font-size: 0.9em; margin-bottom: 10px; "
         "text-transform: uppercase; letter-spacing: 1px; }";
  css += ".value { font-size: 2.5em; font-weight: bold; color: #667eea; "
         "margin-bottom: 5px; }";
  css += ".unit { color: #999; font-size: 0.9em; }";
  css += ".air-quality { background: white; border-radius: 15px; padding: "
         "30px; text-align: center; box-shadow: 0 10px 30px rgba(0,0,0,0.2); "
         "margin-bottom: 20px; }";
  css += ".air-quality h2 { color: #333; margin-bottom: 20px; }";
  css += ".quality-badge { display: inline-block; padding: 15px 30px; "
         "border-radius: 25px; font-size: 1.3em; font-weight: bold; "
         "background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); "
         "color: white; }";
  css += ".iaq { margin-top: 15px; font-size: 1.1em; color: #666; }";
  css += ".iaq span { font-weight: bold; color: #667eea; }";
  css += ".footer { color: rgba(255,255,255,0.8); text-align: center; "
         "font-size: 0.9em; }";
  css += "@media (max-width: 768px) { h1 { font-size: 1.8em; } .cards { "
         "grid-template-columns: 1fr; } }";

  server.send (200, "text/css", css);
}

// Évaluation de la qualité d'air
String
evaluerQualiteAir (float resistance)
{
  if (resistance > 200)
    return "🟢 Excellent";
  else if (resistance > 150)
    return "🟢 Bon";
  else if (resistance > 100)
    return "🟡 Moyen";
  else if (resistance > 50)
    return "🟠 Médiocre";
  else
    return "🔴 Mauvais";
}

// Calcul IAQ simplifié
int
calculerIAQ (float gasResistance, float humidity)
{
  float gasScore = 500.0 - (gasResistance * 2.0);
  if (gasScore < 0)
    gasScore = 0;
  if (gasScore > 400)
    gasScore = 400;

  float humScore = 0;
  if (humidity < 30 || humidity > 70)
    humScore = 50;
  else if (humidity < 35 || humidity > 65)
    humScore = 25;

  int iaq = (int)(gasScore + humScore);
  if (iaq > 500)
    iaq = 500;

  return iaq;
}
