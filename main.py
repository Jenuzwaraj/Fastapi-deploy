from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import List
from datetime import datetime
import motor.motor_asyncio
from bson import ObjectId
import os
import ssl
import certifi

app = FastAPI(title="Sensor Data API")

# MongoDB connection
base_url = os.getenv("MONGODB_URL", "mongodb+srv://priyankapk793_db_user:adrTD6FaFT7y6vVb@cluster0.pmydy7n.mongodb.net")
MONGO_DB_NAME = os.getenv("MONGO_DB_NAME", "sensor_data")

# Add connection parameters to the URL
if "?" not in base_url:
    MONGODB_URL = f"{base_url}/?retryWrites=true&w=majority"
else:
    MONGODB_URL = base_url

# Configure SSL context for MongoDB Atlas
# Note: For mongodb+srv://, TLS is automatically enabled
# The TLSV1_ALERT_INTERNAL_ERROR often indicates IP whitelist issues in MongoDB Atlas
client = motor.motor_asyncio.AsyncIOMotorClient(
    MONGODB_URL,
    serverSelectionTimeoutMS=10000,
    connectTimeoutMS=10000
)
db = client[MONGO_DB_NAME]
collection = db.sensor_readings


class SensorReading(BaseModel):
    sensor_id: int
    sensor_name: str
    reading_type: str
    reading: float
    units: str
    created_at: str
    updated_at: str


class SensorData(BaseModel):
    device_id: int
    readings: List[SensorReading]


class SensorDataResponse(BaseModel):
    status: str
    message: str
    device_id: int


@app.get("/")
async def root():
    return {"message": "Sensor Data API is running"}


@app.post("/sensor-data", response_model=SensorDataResponse)
async def receive_sensor_data(data: SensorData):
    """
    Receive sensor data from microcontroller and save to MongoDB
    """
    try:
        # Prepare document for MongoDB
        document = {
            "device_id": data.device_id,
            "readings": [
                {
                    "sensor_id": reading.sensor_id,
                    "sensor_name": reading.sensor_name,
                    "reading_type": reading.reading_type,
                    "reading": reading.reading,
                    "units": reading.units,
                    "created_at": reading.created_at,
                    "updated_at": reading.updated_at
                }
                for reading in data.readings
            ],
            "created_at": datetime.utcnow().isoformat(),
            "updated_at": datetime.utcnow().isoformat()
        }
        
        # Insert into MongoDB
        result = await collection.insert_one(document)
        
        if result.inserted_id:
            return SensorDataResponse(
                status="success",
                message="Sensor data saved successfully",
                device_id=data.device_id
            )
        else:
            raise HTTPException(status_code=500, detail="Failed to save sensor data")
            
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@app.get("/health")
async def health_check():
    """
    Health check endpoint
    """
    try:
        # Check MongoDB connection
        await db.list_collection_names()
        return {"status": "healthy", "database": "connected"}
    except Exception as e:
        return {"status": "unhealthy", "error": str(e)}
