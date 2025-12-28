// Firebase Data Initialization Script
// Run this once to populate initial data in Firebase

import { ref, set } from 'firebase/database';
import { database } from './firebase';

export const initializeFirebaseData = async () => {
  const now = Date.now();

  // Initialize current sensor values
  await set(ref(database, 'sensors/current'), {
    temperature: 24.5,
    humidity: 85.2,
    co2: 820,
    moisture: 72.8,
    ph: 6.5
  });

  // Initialize sensor history with sample data
  const generateHistory = (baseValue: number, variance: number, count: number = 30) => {
    const history: Record<string, { timestamp: number; value: number }> = {};
    for (let i = 0; i < count; i++) {
      const key = `reading_${i}`;
      history[key] = {
        timestamp: now - (count - 1 - i) * 60000, // Each reading 1 minute apart
        value: baseValue + (Math.random() - 0.5) * variance
      };
    }
    return history;
  };

  await set(ref(database, 'sensors/temperature/history'), generateHistory(24, 4));
  await set(ref(database, 'sensors/humidity/history'), generateHistory(85, 10));
  await set(ref(database, 'sensors/co2/history'), generateHistory(800, 200));
  await set(ref(database, 'sensors/moisture/history'), generateHistory(75, 15));
  await set(ref(database, 'sensors/ph/history'), generateHistory(6.5, 0.8));

  // Initialize ML Model data
  await set(ref(database, 'mlModel'), {
    name: 'Mushroom Fruiting Predictor',
    version: '2.1.0',
    accuracy: 94.5,
    lastTrainedDate: '2025-11-20T10:30:00Z',
    status: 'active',
    predictions: {
      fruitingReadiness: 78,
      estimatedHarvestDate: '2025-12-05',
      healthScore: 92
    },
    features: ['Temperature', 'Humidity', 'CO₂', 'Moisture', 'pH', 'Light Intensity'],
    description: 'CNN-based model for predicting optimal fruiting conditions and disease detection in mushroom cultivation.'
  });

  // Initialize Robot Arm data
  await set(ref(database, 'robotArm'), {
    currentPlot: 1,
    targetPlot: 1,
    status: 'idle',
    lastAction: 'System initialized',
    commandTimestamp: now
  });

  // Initialize Plots
  await set(ref(database, 'plots'), {
    plot_1: { id: 1, name: 'Plot 1', status: 'active', lastVisited: new Date().toISOString() },
    plot_2: { id: 2, name: 'Plot 2', status: 'active', lastVisited: new Date().toISOString() },
    plot_3: { id: 3, name: 'Plot 3', status: 'active', lastVisited: new Date().toISOString() },
    plot_4: { id: 4, name: 'Plot 4', status: 'active', lastVisited: new Date().toISOString() },
    plot_5: { id: 5, name: 'Plot 5', status: 'inactive', lastVisited: new Date().toISOString() },
    plot_6: { id: 6, name: 'Plot 6', status: 'active', lastVisited: new Date().toISOString() }
  });

  // Initialize Light Control
  await set(ref(database, 'lightControl'), {
    intensity: 75,
    isAuto: true,
    status: 'on'
  });

  // Initialize some sample alerts
  await set(ref(database, 'alerts'), {
    alert_1: {
      type: 'info',
      message: 'System started successfully',
      timestamp: now - 3600000,
      acknowledged: true
    },
    alert_2: {
      type: 'warning',
      message: 'CO₂ levels approaching upper threshold',
      timestamp: now - 1800000,
      acknowledged: false
    },
    alert_3: {
      type: 'success',
      message: 'Optimal humidity maintained for 24 hours',
      timestamp: now - 900000,
      acknowledged: false
    }
  });

  // Initialize sensor commands (for ESP32 to read)
  await set(ref(database, 'commands'), {
    sensors: {},
    robotArm: {
      lastCommand: 'none',
      timestamp: now
    }
  });

  console.log('Firebase data initialized successfully!');
};

export default initializeFirebaseData;
