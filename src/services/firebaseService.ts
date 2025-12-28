import { ref, onValue, set, push, update } from 'firebase/database';
import { database } from '../config/firebase';
import { 
  SensorData, 
  CurrentSensorValues, 
  MLModelInfo, 
  RobotArmPosition, 
  Plot, 
  SensorControlCommand,
  LightControl,
  Alert
} from '../types';

// Sensor Data Services
export const subscribeSensorData = (
  sensorType: string,
  callback: (data: SensorData[]) => void
) => {
  const sensorRef = ref(database, `sensors/${sensorType}/history`);
  return onValue(sensorRef, (snapshot) => {
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
  return onValue(sensorRef, (snapshot) => {
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
  return onValue(modelRef, (snapshot) => {
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
export const subscribeRobotArmPosition = (
  callback: (data: RobotArmPosition | null) => void
) => {
  const robotRef = ref(database, 'robotArm');
  return onValue(robotRef, (snapshot) => {
    callback(snapshot.val());
  });
};

export const moveRobotToPlot = async (plotId: number) => {
  const robotRef = ref(database, 'robotArm');
  await update(robotRef, {
    targetPlot: plotId,
    status: 'moving',
    lastAction: `Moving to plot ${plotId}`,
    commandTimestamp: Date.now()
  });
};

export const updateRobotStatus = async (status: 'idle' | 'moving' | 'operating') => {
  const statusRef = ref(database, 'robotArm/status');
  await set(statusRef, status);
};

// Plot Services
export const subscribePlots = (callback: (data: Plot[]) => void) => {
  const plotsRef = ref(database, 'plots');
  return onValue(plotsRef, (snapshot) => {
    const data = snapshot.val();
    if (data) {
      callback(Object.values(data));
    } else {
      callback([]);
    }
  });
};

export const initializePlots = async (numberOfPlots: number) => {
  const plotsRef = ref(database, 'plots');
  const plots: Record<string, Plot> = {};
  for (let i = 1; i <= numberOfPlots; i++) {
    plots[`plot_${i}`] = {
      id: i,
      name: `Plot ${i}`,
      status: 'active',
      lastVisited: new Date().toISOString()
    };
  }
  await set(plotsRef, plots);
};

// Sensor Control Commands
export const sendSensorCommand = async (command: SensorControlCommand) => {
  const commandsRef = ref(database, 'commands/sensors');
  const newCommandRef = push(commandsRef);
  await set(newCommandRef, {
    ...command,
    timestamp: Date.now()
  });
};

export const triggerSensorReading = async (
  sensorType: 'ph' | 'moisture' | 'co2' | 'humidity' | 'temperature'
) => {
  await sendSensorCommand({
    sensorType,
    action: 'read',
    timestamp: Date.now()
  });
};

// Light Control Services
export const subscribeLightControl = (callback: (data: LightControl | null) => void) => {
  const lightRef = ref(database, 'lightControl');
  return onValue(lightRef, (snapshot) => {
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
  return onValue(alertsRef, (snapshot) => {
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
