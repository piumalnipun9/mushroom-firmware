import React, { useState, useEffect } from 'react';
import {
  Box,
  Typography,
  Paper,
  Button,
  Slider,
  Switch,
  FormControlLabel,
  Divider,
  useTheme
} from '@mui/material';
import LightModeIcon from '@mui/icons-material/LightMode';
import WaterDropIcon from '@mui/icons-material/WaterDrop';
import RefreshIcon from '@mui/icons-material/Refresh';
import { 
  updateLightControl, 
  subscribeLightControl,
  subscribeHumidifierControl,
  updateHumidifierMode,
  HumidifierMode
} from '../../services/firebaseService';
import { LightControl } from '../../types';

const HumidifierAndLightControl: React.FC = () => {
  const theme = useTheme();
  const [lightIntensity, setLightIntensity] = useState<number>(75);
  const [isAutoLight, setIsAutoLight] = useState<boolean>(true);
  const [lightStatus, setLightStatus] = useState<boolean>(true);
  const [humidifierMode, setHumidifierMode] = useState<HumidifierMode>('OFF');

  // Subscribe to Firebase light control data
  useEffect(() => {
    const unsubscribeLight = subscribeLightControl((data: LightControl | null) => {
      if (data) {
        setLightIntensity(data.intensity);
        setIsAutoLight(data.isAuto);
        setLightStatus(data.status === 'on');
      }
    });

    return () => {
      unsubscribeLight();
    };
  }, []);

  // Subscribe to Firebase humidifier control data
  useEffect(() => {
    const unsubscribeHumidifier = subscribeHumidifierControl((data) => {
      if (data && data.mode) {
        setHumidifierMode(data.mode);
      }
    });

    return () => {
      unsubscribeHumidifier();
    };
  }, []);

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

  const getHumidifierColor = () => {
    switch (humidifierMode) {
      case 'FAST':
        return '#ef4444';
      case 'SLOW':
        return '#f59e0b';
      case 'OFF':
      default:
        return '#9ca3af';
    }
  };

  const cycleHumidifierMode = async () => {
    const modes: HumidifierMode[] = ['OFF', 'SLOW', 'FAST'];
    const currentIndex = modes.indexOf(humidifierMode);
    const nextMode = modes[(currentIndex + 1) % modes.length];
    setHumidifierMode(nextMode);
    await updateHumidifierMode(nextMode);
    console.log(`Humidifier mode updated to: ${nextMode}`);
  };

  return (
    <Box>
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 2, mb: 3 }}>
        <Box sx={{ display: 'flex', gap: 1 }}>
          <LightModeIcon sx={{ fontSize: 40, color: '#fbbf24' }} />
          <WaterDropIcon sx={{ fontSize: 40, color: '#4ecdc4' }} />
        </Box>
        <Typography 
          variant="h4" 
          color="text.primary"
          sx={{ fontWeight: 700 }}
        >
          Humidifier & Light Control
        </Typography>
      </Box>

      {/* Light Control */}
      <Paper elevation={2} sx={{ p: 3, borderRadius: 2, mb: 3 }}>
        <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600, mb: 3, display: 'flex', alignItems: 'center', gap: 1 }}>
          <LightModeIcon sx={{ color: '#fbbf24' }} />
          Light Control
        </Typography>

        <Box sx={{ display: 'grid', gridTemplateColumns: { xs: '1fr', md: 'repeat(2, 1fr)' }, gap: 3 }}>
          <Box>
            <Typography variant="body2" color="text.secondary" sx={{ mb: 2, fontWeight: 600 }}>
              Light Intensity: {lightIntensity}%
            </Typography>
            <Slider
              value={lightIntensity}
              onChange={handleLightIntensityChange}
              disabled={isAutoLight}
              min={0}
              max={100}
              marks={[
                { value: 0, label: '0%' },
                { value: 50, label: '50%' },
                { value: 100, label: '100%' }
              ]}
              valueLabelDisplay="auto"
              sx={{
                color: '#fbbf24',
                '& .MuiSlider-thumb': {
                  backgroundColor: '#fbbf24',
                  '&:hover': {
                    boxShadow: '0 0 0 8px rgba(251, 191, 36, 0.16)'
                  }
                },
                '& .MuiSlider-track': {
                  backgroundColor: '#fbbf24'
                },
                '& .MuiSlider-rail': {
                  backgroundColor: '#e5e7eb'
                }
              }}
            />
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
                <Typography color="text.secondary" sx={{ fontWeight: 500 }}>
                  Status: {lightStatus ? 'ON' : 'OFF'}
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
                <Typography color="text.secondary" sx={{ fontWeight: 500 }}>
                  Auto Mode (ML Controlled)
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
              ? 'Auto mode enabled - ML model controls light based on fruiting conditions'
              : 'Manual mode - adjust intensity using the slider above'
            }
          </Typography>
        </Box>
      </Paper>

      {/* Humidifier Control */}
      <Paper elevation={2} sx={{ p: 3, borderRadius: 2 }}>
        <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600, mb: 3, display: 'flex', alignItems: 'center', gap: 1 }}>
          <WaterDropIcon sx={{ color: '#4ecdc4' }} />
          Humidifier Control
        </Typography>

        <Box sx={{ display: 'flex', alignItems: 'center', gap: 3, mb: 3 }}>
          <Box
            sx={{
              width: 80,
              height: 80,
              borderRadius: '50%',
              backgroundColor: `${getHumidifierColor()}20`,
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              border: `3px solid ${getHumidifierColor()}`,
              boxShadow: `0 0 20px ${getHumidifierColor()}40`,
              transition: 'all 0.3s ease'
            }}
          >
            <Typography 
              variant="h5" 
              sx={{ 
                fontWeight: 700, 
                color: getHumidifierColor(),
                textAlign: 'center'
              }}
            >
              {humidifierMode}
            </Typography>
          </Box>

          <Box sx={{ flex: 1 }}>
            <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
              Current Mode: <strong>{humidifierMode}</strong>
            </Typography>
            <Typography variant="caption" color="text.secondary" sx={{ display: 'block', mb: 2 }}>
              Cycles: OFF → SLOW → FAST → OFF
            </Typography>
            <Button
              variant="contained"
              startIcon={<RefreshIcon />}
              onClick={cycleHumidifierMode}
              sx={{
                backgroundColor: getHumidifierColor(),
                '&:hover': {
                  backgroundColor: getHumidifierColor(),
                  filter: 'brightness(0.9)'
                }
              }}
            >
              Cycle Mode
            </Button>
          </Box>
        </Box>

        <Divider sx={{ my: 3 }} />

        <Box>
          <Typography variant="body2" color="text.secondary">
            <strong>How it works:</strong> The humidifier cycles through three states - OFF (no moisture), SLOW (gentle misting), and FAST (aggressive misting). Press "Cycle Mode" to advance to the next state, or trigger GPIO15 with a falling edge pulse to cycle automatically.
          </Typography>
        </Box>
      </Paper>
    </Box>
  );
};

export default HumidifierAndLightControl;
