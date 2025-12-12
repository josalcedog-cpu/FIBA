import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
import pandas as pd
import time
import os

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
SERVICE_ACCOUNT_KEY = os.path.join(BASE_DIR, 'serviceAccountKey.json')
OUTPUT_FILE = os.path.join(BASE_DIR, 'fiba_data.xlsx')
DATABASE_URL = 'https://fiba-bc93c-default-rtdb.firebaseio.com'
UPDATE_INTERVAL = 60

def main():
    print("--- INICIANDO SISTEMA DE BIG DATA FIBA ---")

    if not os.path.exists(fiba-bc93c-firebase-adminsdk-fbsvc-b8a534e06b.json):
        print(f"ERROR: No se encuentra el archivo '{SERVICE_ACCOUNT_KEY}'")
        print("Por favor descarga la llave privada desde Firebase Console y ponla en esta carpeta.")
        return

    try:
        cred = credentials.Certificate(SERVICE_ACCOUNT_KEY)
        firebase_admin.initialize_app(cred, {
            'databaseURL': DATABASE_URL
        })
        print(">> Conexión a Firebase Exitosa.")
    except Exception as e:
        print(f"ERROR conectando a Firebase: {e}")
        return

    print(f">> Escuchando cambios en nodo '/medidas'. Actualizando Excel cada {UPDATE_INTERVAL}s...")
    
    while True:
        try:
            ref = db.reference('/medidas')
            data = ref.get()

            if data:
                data_list = []
                for key, value in data.items():
                    record = value
                    record['firebase_id'] = key
                    data_list.append(record)
                
                df = pd.DataFrame(data_list)

                cols = ['timestamp', 'co2_ppm', 'gas_kohm', 'temperature', 'pm25', 'pm10', 'firebase_id']
                existing_cols = [c for c in cols if c in df.columns]
                remaining_cols = [c for c in df.columns if c not in existing_cols]
                df = df[existing_cols + remaining_cols]

                df.to_excel(OUTPUT_FILE, index=False)
                
                print(f"[{time.strftime('%H:%M:%S')}] Excel actualizado: {OUTPUT_FILE} ({len(df)} registros)")
            else:
                print(f"[{time.strftime('%H:%M:%S')}] La base de datos está vacía.")

        except Exception as e:
            print(f"ERROR en ciclo de actualización: {e}")

        time.sleep(UPDATE_INTERVAL)

if __name__ == '__main__':
    main()
