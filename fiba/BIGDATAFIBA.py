import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
import pandas as pd
import time
import os

# ==============================================================================
# CONFIGURACION
# ==============================================================================
# 1. Necesitas el archivo de credenciales de Firebase.
#    - Ve a la Consola de Firebase > Project Settings > Service Accounts.
#    - Genera una nueva Private Key (json).
#    - Guardala en esta misma carpeta con el nombre: "serviceAccountKey.json"
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
SERVICE_ACCOUNT_KEY = os.path.join(BASE_DIR, 'serviceAccountKey.json')
OUTPUT_FILE = os.path.join(BASE_DIR, 'fiba_data.xlsx')


# 2. URL de tu Base de Datos (Tomada de tu codigo Arduino)
DATABASE_URL = 'https://fiba-bc93c-default-rtdb.firebaseio.com'

# 3. Nombre del archivo Excel de salida
OUTPUT_FILE = 'fiba_data.xlsx'

# Tiempo de actualizacion (segundos)
UPDATE_INTERVAL = 60
# ==============================================================================

def main():
    print("--- INICIANDO SISTEMA DE BIG DATA FIBA ---")

    # 1. Verificar credenciales
    if not os.path.exists(fiba-bc93c-firebase-adminsdk-fbsvc-b8a534e06b.json):
        print(f"ERROR: No se encuentra el archivo '{SERVICE_ACCOUNT_KEY}'")
        print("Por favor descarga la llave privada desde Firebase Console y ponla en esta carpeta.")
        return

    # 2. Inicializar Firebase
    try:
        cred = credentials.Certificate(SERVICE_ACCOUNT_KEY)
        firebase_admin.initialize_app(cred, {
            'databaseURL': DATABASE_URL
        })
        print(">> Conexión a Firebase Exitosa.")
    except Exception as e:
        print(f"ERROR conectando a Firebase: {e}")
        return

    # 3. Ciclo principal
    print(f">> Escuchando cambios en nodo '/medidas'. Actualizando Excel cada {UPDATE_INTERVAL}s...")
    
    while True:
        try:
            # Obtener datos
            ref = db.reference('/medidas')
            data = ref.get()

            if data:
                # Firebase devuelve un diccionario { "ID_UNICO": {datos...}, "ID2": {datos...} }
                # Convertimos esto a una lista de diccionarios para Pandas
                data_list = []
                for key, value in data.items():
                    # Agregamos el ID de Firebase al registro por si es util
                    record = value
                    record['firebase_id'] = key
                    data_list.append(record)
                
                # Crear DataFrame
                df = pd.DataFrame(data_list)

                # (Opcional) Reordenar columnas para que timestamp quede primero
                cols = ['timestamp', 'co2_ppm', 'gas_kohm', 'temperature', 'pm25', 'pm10', 'firebase_id']
                # Filtramos solo las columnas que existan en el df (para evitar errores si falta alguna)
                existing_cols = [c for c in cols if c in df.columns]
                # Agregamos las que sobren al final
                remaining_cols = [c for c in df.columns if c not in existing_cols]
                df = df[existing_cols + remaining_cols]

                # Guardar a Excel
                df.to_excel(OUTPUT_FILE, index=False)
                
                print(f"[{time.strftime('%H:%M:%S')}] Excel actualizado: {OUTPUT_FILE} ({len(df)} registros)")
            else:
                print(f"[{time.strftime('%H:%M:%S')}] La base de datos está vacía.")

        except Exception as e:
            print(f"ERROR en ciclo de actualización: {e}")

        # Esperar
        time.sleep(UPDATE_INTERVAL)

if __name__ == '__main__':
    main()
