export interface SensorData {
  timestamp: number;
  value: number;
}

export interface SensorReadings {
  ph: SensorData[];
  moisture: SensorData[];
  co2: SensorData[];
  humidity: SensorData[];
  temperature: SensorData[];
}

export interface CurrentSensorValues {
  ph: number;
  moisture: number;
  co2: number;
  humidity: number;
  temperature: number;
}

export interface MLModelInfo {
  name: string;
  version: string;
  accuracy: number;
  lastTrainedDate: string;
  status: 'active' | 'inactive' | 'training';
  predictions: {
    fruitingReadiness: number;
    estimatedHarvestDate: string;
    healthScore: number;
  };
  features: string[];
  description: string;
}

export interface RobotArmPosition {
  currentPlot: number;
  status: 'idle' | 'moving' | 'operating';
  lastAction: string;
}

export interface Plot {
  id: number;
  name: string;
  status: 'active' | 'inactive';
  lastVisited: string;
}

export interface SensorControlCommand {
  sensorType: 'ph' | 'moisture' | 'co2' | 'humidity' | 'temperature';
  action: 'read' | 'calibrate';
  timestamp: number;
}

export interface LightControl {
  intensity: number;
  isAuto: boolean;
  status: 'on' | 'off';
}

export interface Alert {
  id: string;
  type: 'warning' | 'error' | 'info' | 'success';
  message: string;
  timestamp: number;
  acknowledged: boolean;
}
