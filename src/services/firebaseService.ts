import { ref, onValue, set, push, update, DataSnapshot } from 'firebase/database';
import { database } from '../config/firebase';
import {
  SensorData,
  CurrentSensorValues,
  MLModelInfo,
  RobotArmStatus,
  RobotArmCommand,
  LightControl,
  Alert
} from '../types';

// Sensor Data Services
export const subscribeSensorData = (
  sensorType: string,
  callback: (data: SensorData[]) => void
) => {
  const sensorRef = ref(database, `sensors/${sensorType}/history`);
  return onValue(sensorRef, (snapshot: DataSnapshot) => {
    const data = snapshot.val();
    if (data) {
      const dataArray = Object.values(data) as SensorData[];
      callback(dataArray.slice(-50)); // Last 50 readings
    } else {
      callback([]);
    }
  });
};

export const subscribeCurrentSensorValues = (
  callback: (data: CurrentSensorValues) => void
) => {
  const sensorRef = ref(database, 'sensors/current');
  return onValue(sensorRef, (snapshot: DataSnapshot) => {
    const data = snapshot.val();
    if (data) {
      callback(data);
    } else {
      callback({
        ph: 0,
        moisture: 0,
        co2: 0,
        humidity: 0,
        temperature: 0
      });
    }
  });
};

export const writeSensorReading = async (
  sensorType: string,
  value: number
) => {
  const historyRef = ref(database, `sensors/${sensorType}/history`);
  const newReadingRef = push(historyRef);
  await set(newReadingRef, {
    timestamp: Date.now(),
    value
  });

  const currentRef = ref(database, `sensors/current/${sensorType}`);
  await set(currentRef, value);
};

// ML Model Services
export const subscribeMLModelInfo = (
  callback: (data: MLModelInfo | null) => void
) => {
  const modelRef = ref(database, 'mlModel');
  return onValue(modelRef, (snapshot: DataSnapshot) => {
    callback(snapshot.val());
  });
};

export const updateMLModelStatus = async (status: 'active' | 'inactive' | 'training') => {
  const statusRef = ref(database, 'mlModel/status');
  await set(statusRef, status);
};

export const updateMLModelPredictions = async (predictions: MLModelInfo['predictions']) => {
  const predictionsRef = ref(database, 'mlModel/predictions');
  await set(predictionsRef, predictions);
};

// Robot Arm Services
// UI reads firmware state from robotArm/status/
export const subscribeRobotArmStatus = (
  callback: (data: RobotArmStatus | null) => void
) => {
  const robotRef = ref(database, 'robotArm/status');
  return onValue(robotRef, (snapshot: DataSnapshot) => {
    callback(snapshot.val());
  });
};

// UI sends commands to robotArm/command/ only; firmware reads + clears
export const sendRobotCommand = async (
  action: RobotArmCommand['action'],
  targetPlot?: number
) => {
  const cmdRef = ref(database, 'robotArm/command');
  await update(cmdRef, {
    action,
    ...(targetPlot !== undefined ? { targetPlot } : {}),
    timestamp: Date.now()
  });
};

// Legacy alias — kept so other components don't break immediately
export const subscribeRobotArmPosition = subscribeRobotArmStatus;
export const moveRobotToPlot = (plotId: number) => sendRobotCommand('move', plotId);
export const updateRobotStatus = (_status: string) => sendRobotCommand('stop');



// Humidifier Control Services
export type HumidifierMode = 'ON' | 'OFF';

export interface HumidifierControl {
  mode: HumidifierMode;
  lastUpdated: number;
}

export const subscribeHumidifierControl = (callback: (data: HumidifierControl | null) => void) => {
  const humidifierRef = ref(database, 'humidifier');
  return onValue(humidifierRef, (snapshot: DataSnapshot) => {
    callback(snapshot.val());
  });
};

export const updateHumidifierMode = async (mode: HumidifierMode) => {
  const humidifierRef = ref(database, 'humidifier');
  await update(humidifierRef, {
    mode,
    lastUpdated: Date.now()
  });
};

// Light Control Services
export const subscribeLightControl = (callback: (data: LightControl | null) => void) => {
  const lightRef = ref(database, 'lightControl');
  return onValue(lightRef, (snapshot: DataSnapshot) => {
    callback(snapshot.val());
  });
};

export const updateLightControl = async (control: Partial<LightControl>) => {
  const lightRef = ref(database, 'lightControl');
  await update(lightRef, control);
};

// Alert Services
export const subscribeAlerts = (callback: (data: Alert[]) => void) => {
  const alertsRef = ref(database, 'alerts');
  return onValue(alertsRef, (snapshot: DataSnapshot) => {
    const data = snapshot.val();
    if (data) {
      callback(Object.entries(data).map(([id, alert]) => ({
        ...(alert as Alert),
        id
      })));
    } else {
      callback([]);
    }
  });
};

export const acknowledgeAlert = async (alertId: string) => {
  const alertRef = ref(database, `alerts/${alertId}/acknowledged`);
  await set(alertRef, true);
};

export const createAlert = async (alert: Omit<Alert, 'id' | 'timestamp' | 'acknowledged'>) => {
  const alertsRef = ref(database, 'alerts');
  const newAlertRef = push(alertsRef);
  await set(newAlertRef, {
    ...alert,
    timestamp: Date.now(),
    acknowledged: false
  });
};

// Camera URL Service
export const subscribeCameraUrl = (callback: (url: string | null) => void) => {
  const cameraRef = ref(database, 'camera/cameraUrl');
  return onValue(cameraRef, (snapshot: DataSnapshot) => {
    const url = snapshot.val();
    callback(url || null);
  });
};

// Camera Base64 Frame Service — subscribes to camera/frame (raw base64 JPEG string)
// The ESP32-CAM firmware pushes a new frame at frameUploadFps (default 2fps)
export const subscribeCameraFrame = (callback: (frame: string | null) => void) => {
  const frameRef = ref(database, 'camera/frame');
  return onValue(frameRef, (snapshot: DataSnapshot) => {
    const frame = snapshot.val();
    callback(frame ? String(frame) : null);
  });
};
