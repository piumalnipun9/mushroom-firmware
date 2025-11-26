import React, { useEffect, useState } from 'react';
import { Box, Typography, CircularProgress, Button, Paper } from '@mui/material';
import RefreshIcon from '@mui/icons-material/Refresh';
import DashboardIcon from '@mui/icons-material/Dashboard';
import TimelineIcon from '@mui/icons-material/Timeline';
import SensorChart from './SensorChart';
import SensorCard from './SensorCard';
import VideoFeed from './VideoFeed';
import { SensorData, CurrentSensorValues } from '../../types';
import { subscribeSensorData, subscribeCurrentSensorValues } from '../../services/firebaseService';
import { initializeFirebaseData } from '../../config/initFirebaseData';

const Dashboard: React.FC = () => {
  const [phData, setPhData] = useState<SensorData[]>([]);
  const [moistureData, setMoistureData] = useState<SensorData[]>([]);
  const [co2Data, setCo2Data] = useState<SensorData[]>([]);
  const [humidityData, setHumidityData] = useState<SensorData[]>([]);
  const [temperatureData, setTemperatureData] = useState<SensorData[]>([]);
  const [currentValues, setCurrentValues] = useState<CurrentSensorValues>({
    ph: 0,
    moisture: 0,
    co2: 0,
    humidity: 0,
    temperature: 0
  });
  const [loading, setLoading] = useState(true);
  const [dataInitialized, setDataInitialized] = useState(false);

  useEffect(() => {
    // Subscribe to sensor data from Firebase
    const unsubscribePh = subscribeSensorData('ph', (data) => {
      setPhData(data);
      setLoading(false);
    });
    const unsubscribeMoisture = subscribeSensorData('moisture', setMoistureData);
    const unsubscribeCo2 = subscribeSensorData('co2', setCo2Data);
    const unsubscribeHumidity = subscribeSensorData('humidity', setHumidityData);
    const unsubscribeTemperature = subscribeSensorData('temperature', setTemperatureData);
    const unsubscribeCurrent = subscribeCurrentSensorValues((data) => {
      setCurrentValues(data);
      if (data.temperature > 0) {
        setDataInitialized(true);
      }
    });

    // Set loading to false after a timeout if no data
    const timeout = setTimeout(() => {
      setLoading(false);
    }, 3000);

    return () => {
      unsubscribePh();
      unsubscribeMoisture();
      unsubscribeCo2();
      unsubscribeHumidity();
      unsubscribeTemperature();
      unsubscribeCurrent();
      clearTimeout(timeout);
    };
  }, []);

  const handleInitializeData = async () => {
    setLoading(true);
    try {
      await initializeFirebaseData();
      setDataInitialized(true);
    } catch (error) {
      console.error('Error initializing Firebase data:', error);
    }
    setLoading(false);
  };

  if (loading) {
    return (
      <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', minHeight: '50vh', gap: 2 }}>
        <CircularProgress />
        <Typography color="text.secondary">Loading sensor data from Firebase...</Typography>
      </Box>
    );
  }

  if (!dataInitialized && currentValues.temperature === 0) {
    return (
      <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', minHeight: '50vh', gap: 3 }}>
        <Typography variant="h5" color="text.primary" sx={{ textAlign: 'center' }}>
          No Data in Firebase
        </Typography>
        <Typography color="text.secondary" sx={{ textAlign: 'center', maxWidth: 500 }}>
          Firebase database is empty. Click the button below to initialize sample data, 
          or your ESP32 will update the values when connected.
        </Typography>
        <Button
          variant="contained"
          size="large"
          startIcon={<RefreshIcon />}
          onClick={handleInitializeData}
          color="success"
          sx={{ px: 4, py: 1.5 }}
        >
          Initialize Sample Data
        </Button>
      </Box>
    );
  }

  return (
    <Box>
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 3 }}>
        <DashboardIcon color="primary" />
        <Typography 
          variant="h4" 
          sx={{ fontWeight: 700 }}
          color="text.primary"
        >
          Mushroom Farm Dashboard
        </Typography>
      </Box>

      {/* Video Feed and Quick Stats Section */}
      <Box 
        sx={{ 
          display: 'grid', 
          gridTemplateColumns: { xs: '1fr', lg: '1fr 1fr' },
          gap: 3,
          mb: 4
        }}
      >
        <VideoFeed />
        
        {/* Quick Stats Summary */}
        <Paper 
          elevation={2}
          sx={{ 
            p: 3, 
            borderRadius: 2,
            display: 'flex',
            flexDirection: 'column',
            justifyContent: 'center'
          }}
        >
          <Typography variant="h6" color="text.primary" fontWeight={600} gutterBottom>
            System Status
          </Typography>
          <Box sx={{ display: 'grid', gridTemplateColumns: 'repeat(2, 1fr)', gap: 2, mt: 1 }}>
            <Box sx={{ p: 2, bgcolor: 'action.hover', borderRadius: 1 }}>
              <Typography variant="body2" color="text.secondary">Temperature</Typography>
              <Typography variant="h5" color="text.primary" fontWeight={600}>{currentValues.temperature}°C</Typography>
            </Box>
            <Box sx={{ p: 2, bgcolor: 'action.hover', borderRadius: 1 }}>
              <Typography variant="body2" color="text.secondary">Humidity</Typography>
              <Typography variant="h5" color="text.primary" fontWeight={600}>{currentValues.humidity}%</Typography>
            </Box>
            <Box sx={{ p: 2, bgcolor: 'action.hover', borderRadius: 1 }}>
              <Typography variant="body2" color="text.secondary">CO2 Level</Typography>
              <Typography variant="h5" color="text.primary" fontWeight={600}>{currentValues.co2} ppm</Typography>
            </Box>
            <Box sx={{ p: 2, bgcolor: 'action.hover', borderRadius: 1 }}>
              <Typography variant="body2" color="text.secondary">pH Level</Typography>
              <Typography variant="h5" color="text.primary" fontWeight={600}>{currentValues.ph}</Typography>
            </Box>
          </Box>
        </Paper>
      </Box>

      {/* Current Sensor Values Cards */}
      <Box 
        sx={{ 
          display: 'grid', 
          gridTemplateColumns: { xs: '1fr', sm: 'repeat(2, 1fr)', md: 'repeat(5, 1fr)' },
          gap: 2,
          mb: 4
        }}
      >
        <SensorCard
          title="Temperature"
          value={currentValues.temperature}
          unit="°C"
          icon="temperature"
          color="#ff6b6b"
          minValue={10}
          maxValue={40}
          optimalMin={20}
          optimalMax={28}
        />
        <SensorCard
          title="Humidity"
          value={currentValues.humidity}
          unit="%"
          icon="humidity"
          color="#4ecdc4"
          minValue={0}
          maxValue={100}
          optimalMin={80}
          optimalMax={95}
        />
        <SensorCard
          title="CO₂ Level"
          value={currentValues.co2}
          unit="ppm"
          icon="co2"
          color="#a78bfa"
          minValue={0}
          maxValue={2000}
          optimalMin={500}
          optimalMax={1000}
        />
        <SensorCard
          title="Moisture"
          value={currentValues.moisture}
          unit="%"
          icon="moisture"
          color="#60a5fa"
          minValue={0}
          maxValue={100}
          optimalMin={65}
          optimalMax={85}
        />
        <SensorCard
          title="pH Level"
          value={currentValues.ph}
          unit="pH"
          icon="ph"
          color="#fbbf24"
          minValue={0}
          maxValue={14}
          optimalMin={6.0}
          optimalMax={7.0}
        />
      </Box>

      {/* Sensor Charts */}
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
        <TimelineIcon color="primary" />
        <Typography 
          variant="h5" 
          sx={{ fontWeight: 600 }}
          color="text.primary"
        >
          Real-time Sensor Data
        </Typography>
      </Box>
      
      <Box 
        sx={{ 
          display: 'grid', 
          gridTemplateColumns: { xs: '1fr', md: 'repeat(2, 1fr)' },
          gap: 3
        }}
      >
        <SensorChart
          title="Temperature"
          data={temperatureData}
          dataKey="temperature"
          color="#ff6b6b"
          unit="°C"
          minValue={10}
          maxValue={40}
        />
        <SensorChart
          title="Humidity"
          data={humidityData}
          dataKey="humidity"
          color="#4ecdc4"
          unit="%"
          minValue={0}
          maxValue={100}
        />
        <SensorChart
          title="CO₂ Level"
          data={co2Data}
          dataKey="co2"
          color="#a78bfa"
          unit="ppm"
          minValue={0}
          maxValue={2000}
        />
        <SensorChart
          title="Moisture"
          data={moistureData}
          dataKey="moisture"
          color="#60a5fa"
          unit="%"
          minValue={0}
          maxValue={100}
        />
        <Box sx={{ gridColumn: { xs: '1', md: '1 / -1' } }}>
          <SensorChart
            title="pH Level"
            data={phData}
            dataKey="ph"
            color="#fbbf24"
            unit="pH"
            minValue={0}
            maxValue={14}
          />
        </Box>
      </Box>
    </Box>
  );
};

export default Dashboard;
