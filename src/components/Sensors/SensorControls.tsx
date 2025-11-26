import React, { useState, useEffect } from 'react';
import {
  Box,
  Typography,
  Paper,
  Button,
  CircularProgress,
  Slider,
  Switch,
  FormControlLabel,
  Divider,
  useTheme
} from '@mui/material';
import SensorsIcon from '@mui/icons-material/Sensors';
import ThermostatIcon from '@mui/icons-material/Thermostat';
import WaterDropIcon from '@mui/icons-material/WaterDrop';
import Co2Icon from '@mui/icons-material/Co2';
import OpacityIcon from '@mui/icons-material/Opacity';
import ScienceIcon from '@mui/icons-material/Science';
import LightModeIcon from '@mui/icons-material/LightMode';
import RefreshIcon from '@mui/icons-material/Refresh';
import BuildIcon from '@mui/icons-material/Build';
import DownloadIcon from '@mui/icons-material/Download';
import { 
  triggerSensorReading, 
  updateLightControl, 
  subscribeCurrentSensorValues,
  subscribeLightControl 
} from '../../services/firebaseService';
import { CurrentSensorValues, LightControl } from '../../types';

interface SensorControlItemProps {
  title: string;
  icon: React.ReactNode;
  color: string;
  onRead: () => void;
  onCalibrate: () => void;
  isReading: boolean;
  lastReading?: number;
  unit: string;
}

const SensorControlItem: React.FC<SensorControlItemProps> = ({
  title,
  icon,
  color,
  onRead,
  onCalibrate,
  isReading,
  lastReading,
  unit
}) => {
  return (
    <Paper elevation={2} sx={{ p: 2.5, borderRadius: 2 }}>
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 2, mb: 2 }}>
        <Box
          sx={{
            width: 48,
            height: 48,
            borderRadius: 2,
            background: `${color}20`,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center'
          }}
        >
          {icon}
        </Box>
        <Box sx={{ flex: 1 }}>
          <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600 }}>
            {title}
          </Typography>
          {lastReading !== undefined && (
            <Typography variant="body2" color="text.secondary">
              Last: {lastReading.toFixed(2)} {unit}
            </Typography>
          )}
        </Box>
      </Box>
      
      <Box sx={{ display: 'flex', gap: 1 }}>
        <Button
          variant="contained"
          size="small"
          startIcon={isReading ? <CircularProgress size={16} color="inherit" /> : <RefreshIcon />}
          onClick={onRead}
          disabled={isReading}
          sx={{
            flex: 1,
            backgroundColor: color,
            '&:hover': {
              backgroundColor: color,
              filter: 'brightness(0.9)'
            }
          }}
        >
          {isReading ? 'Reading...' : 'Read'}
        </Button>
        <Button
          variant="outlined"
          size="small"
          startIcon={<BuildIcon />}
          onClick={onCalibrate}
          sx={{
            borderColor: color,
            color: color,
            '&:hover': {
              borderColor: color,
              backgroundColor: `${color}10`
            }
          }}
        >
          Calibrate
        </Button>
      </Box>
    </Paper>
  );
};

const SensorControls: React.FC = () => {
  const theme = useTheme();
  const [readingStates, setReadingStates] = useState<Record<string, boolean>>({});
  const [lightIntensity, setLightIntensity] = useState<number>(75);
  const [isAutoLight, setIsAutoLight] = useState<boolean>(true);
  const [lightStatus, setLightStatus] = useState<boolean>(true);
  const [currentValues, setCurrentValues] = useState<CurrentSensorValues>({
    ph: 0,
    moisture: 0,
    co2: 0,
    humidity: 0,
    temperature: 0
  });

  // Subscribe to Firebase data
  useEffect(() => {
    const unsubscribeSensors = subscribeCurrentSensorValues(setCurrentValues);
    const unsubscribeLight = subscribeLightControl((data: LightControl | null) => {
      if (data) {
        setLightIntensity(data.intensity);
        setIsAutoLight(data.isAuto);
        setLightStatus(data.status === 'on');
      }
    });

    return () => {
      unsubscribeSensors();
      unsubscribeLight();
    };
  }, []);

  const handleSensorRead = async (sensorType: 'ph' | 'moisture' | 'co2' | 'humidity' | 'temperature') => {
    setReadingStates(prev => ({ ...prev, [sensorType]: true }));
    await triggerSensorReading(sensorType);
    
    // Simulate reading time
    setTimeout(() => {
      setReadingStates(prev => ({ ...prev, [sensorType]: false }));
    }, 2000);
  };

  const handleCalibrate = (sensorType: string) => {
    console.log(`Calibrating ${sensorType} sensor`);
    // Implement calibration logic
  };

  const handleLightIntensityChange = async (_: Event, value: number | number[]) => {
    const intensity = value as number;
    setLightIntensity(intensity);
    await updateLightControl({ intensity });
  };

  const handleAutoLightToggle = async (event: React.ChangeEvent<HTMLInputElement>) => {
    setIsAutoLight(event.target.checked);
    await updateLightControl({ isAuto: event.target.checked });
  };

  const handleLightStatusToggle = async (event: React.ChangeEvent<HTMLInputElement>) => {
    setLightStatus(event.target.checked);
    await updateLightControl({ status: event.target.checked ? 'on' : 'off' });
  };

  const sensors = [
    {
      type: 'temperature' as const,
      title: 'Temperature',
      icon: <ThermostatIcon sx={{ color: '#ff6b6b', fontSize: 28 }} />,
      color: '#ff6b6b',
      unit: '°C',
      lastReading: currentValues.temperature
    },
    {
      type: 'humidity' as const,
      title: 'Humidity',
      icon: <OpacityIcon sx={{ color: '#4ecdc4', fontSize: 28 }} />,
      color: '#4ecdc4',
      unit: '%',
      lastReading: currentValues.humidity
    },
    {
      type: 'co2' as const,
      title: 'CO2 Level',
      icon: <Co2Icon sx={{ color: '#a78bfa', fontSize: 28 }} />,
      color: '#a78bfa',
      unit: 'ppm',
      lastReading: currentValues.co2
    },
    {
      type: 'moisture' as const,
      title: 'Moisture',
      icon: <WaterDropIcon sx={{ color: '#60a5fa', fontSize: 28 }} />,
      color: '#60a5fa',
      unit: '%',
      lastReading: currentValues.moisture
    },
    {
      type: 'ph' as const,
      title: 'pH Level',
      icon: <ScienceIcon sx={{ color: '#fbbf24', fontSize: 28 }} />,
      color: '#fbbf24',
      unit: 'pH',
      lastReading: currentValues.ph
    }
  ];

  return (
    <Box>
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 2, mb: 3 }}>
        <SensorsIcon sx={{ fontSize: 40, color: '#4ecdc4' }} />
        <Typography 
          variant="h4" 
          color="text.primary"
          sx={{ fontWeight: 700 }}
        >
          Sensor Controls
        </Typography>
      </Box>

      {/* Sensor Grid */}
      <Box 
        sx={{ 
          display: 'grid', 
          gridTemplateColumns: { xs: '1fr', sm: 'repeat(2, 1fr)', lg: 'repeat(3, 1fr)' },
          gap: 2,
          mb: 4
        }}
      >
        {sensors.map((sensor) => (
          <SensorControlItem
            key={sensor.type}
            title={sensor.title}
            icon={sensor.icon}
            color={sensor.color}
            onRead={() => handleSensorRead(sensor.type)}
            onCalibrate={() => handleCalibrate(sensor.type)}
            isReading={readingStates[sensor.type] || false}
            lastReading={sensor.lastReading}
            unit={sensor.unit}
          />
        ))}
      </Box>

      {/* Light Control */}
      <Paper elevation={2} sx={{ p: 3, borderRadius: 2 }}>
        <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600, mb: 3, display: 'flex', alignItems: 'center', gap: 1 }}>
          <LightModeIcon sx={{ color: '#fbbf24' }} />
          Light Control Unit
        </Typography>

        <Box sx={{ display: 'grid', gridTemplateColumns: { xs: '1fr', md: 'repeat(2, 1fr)' }, gap: 3 }}>
          <Box>
            <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
              Light Intensity: {lightIntensity}%
            </Typography>
            <Slider
              value={lightIntensity}
              onChange={handleLightIntensityChange}
              disabled={isAutoLight}
              min={0}
              max={100}
              sx={{
                color: '#fbbf24',
                '& .MuiSlider-thumb': {
                  backgroundColor: '#fbbf24'
                },
                '& .MuiSlider-track': {
                  backgroundColor: '#fbbf24'
                }
              }}
            />
            <Box sx={{ display: 'flex', justifyContent: 'space-between', mt: 1 }}>
              <Typography variant="caption" color="text.secondary">0%</Typography>
              <Typography variant="caption" color="text.secondary">100%</Typography>
            </Box>
          </Box>

          <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            <FormControlLabel
              control={
                <Switch
                  checked={lightStatus}
                  onChange={handleLightStatusToggle}
                  color="warning"
                />
              }
              label={
                <Typography color="text.secondary">
                  Light Status: {lightStatus ? 'ON' : 'OFF'}
                </Typography>
              }
            />

            <FormControlLabel
              control={
                <Switch
                  checked={isAutoLight}
                  onChange={handleAutoLightToggle}
                  color="secondary"
                />
              }
              label={
                <Typography color="text.secondary">
                  Auto Mode (ML Controlled - 75%)
                </Typography>
              }
            />
          </Box>
        </Box>

        <Divider sx={{ my: 3 }} />

        <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
          <Box
            sx={{
              width: 12,
              height: 12,
              borderRadius: '50%',
              backgroundColor: lightStatus ? '#fbbf24' : theme.palette.action.disabled,
              boxShadow: lightStatus ? '0 0 10px #fbbf24' : 'none'
            }}
          />
          <Typography variant="body2" color="text.secondary">
            {isAutoLight 
              ? 'Light is controlled by ML model predictions for optimal fruiting conditions'
              : 'Manual control mode - adjust intensity using the slider above'
            }
          </Typography>
        </Box>
      </Paper>

      {/* Quick Actions */}
      <Paper elevation={2} sx={{ p: 3, mt: 3, borderRadius: 2 }}>
        <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600, mb: 3 }}>
          Quick Actions
        </Typography>

        <Box sx={{ display: 'flex', gap: 2, flexWrap: 'wrap' }}>
          <Button
            variant="contained"
            startIcon={<RefreshIcon />}
            onClick={() => {
              sensors.forEach(s => handleSensorRead(s.type));
            }}
            color="success"
          >
            Read All Sensors
          </Button>
          <Button
            variant="outlined"
            startIcon={<BuildIcon />}
            color="warning"
          >
            Calibrate All
          </Button>
          <Button
            variant="outlined"
            startIcon={<DownloadIcon />}
            color="info"
          >
            Export Data
          </Button>
        </Box>
      </Paper>
    </Box>
  );
};

export default SensorControls;
