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

export interface RobotArmStatus {
  state: 'idle' | 'moving' | 'homing' | 'stopping';
  currentPlot: number;   // 0 = home, 1–4 = plots
  lastAction: string;
}

export interface RobotArmCommand {
  action: 'none' | 'move' | 'home' | 'stop';
  targetPlot?: number;
  timestamp?: number;
}

// Keep for legacy references during transitional period
export type RobotArmPosition = RobotArmStatus;


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
