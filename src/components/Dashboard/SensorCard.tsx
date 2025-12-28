import React from 'react';
import { Paper, Typography, Box, useTheme } from '@mui/material';
import ThermostatIcon from '@mui/icons-material/Thermostat';
import WaterDropIcon from '@mui/icons-material/WaterDrop';
import Co2Icon from '@mui/icons-material/Co2';
import OpacityIcon from '@mui/icons-material/Opacity';
import ScienceIcon from '@mui/icons-material/Science';

interface SensorCardProps {
  title: string;
  value: number;
  unit: string;
  icon: 'temperature' | 'humidity' | 'co2' | 'moisture' | 'ph';
  color: string;
  minValue?: number;
  maxValue?: number;
  optimalMin?: number;
  optimalMax?: number;
}

const iconMap = {
  temperature: ThermostatIcon,
  humidity: OpacityIcon,
  co2: Co2Icon,
  moisture: WaterDropIcon,
  ph: ScienceIcon
};

const SensorCard: React.FC<SensorCardProps> = ({
  title,
  value,
  unit,
  icon,
  color,
  minValue = 0,
  maxValue = 100,
  optimalMin,
  optimalMax
}) => {
  const theme = useTheme();
  const Icon = iconMap[icon];
  const progress = ((value - minValue) / (maxValue - minValue)) * 100;
  
  const isOptimal = optimalMin !== undefined && optimalMax !== undefined
    ? value >= optimalMin && value <= optimalMax
    : true;

  const statusColor = isOptimal ? '#4caf50' : '#ff9800';

  return (
    <Paper
      elevation={3}
      sx={{
        p: 2.5,
        borderRadius: 2,
        border: `1px solid ${theme.palette.divider}`,
        transition: 'transform 0.2s, box-shadow 0.2s',
        '&:hover': {
          transform: 'translateY(-4px)',
          boxShadow: theme.shadows[8]
        }
      }}
    >
      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', mb: 2 }}>
        <Box>
          <Typography 
            variant="body2" 
            color="text.secondary"
            sx={{ mb: 0.5 }}
          >
            {title}
          </Typography>
          <Typography 
            variant="h4" 
            color="text.primary"
            sx={{ 
              fontWeight: 700,
              display: 'flex',
              alignItems: 'baseline',
              gap: 0.5
            }}
          >
            {value.toFixed(1)}
            <Typography 
              component="span" 
              color="text.secondary"
              sx={{ fontSize: '1rem' }}
            >
              {unit}
            </Typography>
          </Typography>
        </Box>
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
          <Icon sx={{ color, fontSize: 28 }} />
        </Box>
      </Box>

      <Box sx={{ position: 'relative', mt: 2 }}>
        <Box
          sx={{
            width: '100%',
            height: 6,
            borderRadius: 3,
            background: theme.palette.action.hover,
            overflow: 'hidden'
          }}
        >
          <Box
            sx={{
              width: `${Math.min(Math.max(progress, 0), 100)}%`,
              height: '100%',
              borderRadius: 3,
              background: `linear-gradient(90deg, ${color}80, ${color})`,
              transition: 'width 0.5s ease'
            }}
          />
        </Box>
        {optimalMin !== undefined && optimalMax !== undefined && (
          <Box 
            sx={{ 
              display: 'flex', 
              justifyContent: 'space-between', 
              mt: 1,
              alignItems: 'center'
            }}
          >
            <Typography variant="caption" color="text.secondary">
              Range: {minValue} - {maxValue}
            </Typography>
            <Box 
              sx={{ 
                display: 'flex', 
                alignItems: 'center', 
                gap: 0.5,
                px: 1,
                py: 0.25,
                borderRadius: 1,
                background: `${statusColor}20`
              }}
            >
              <Box 
                sx={{ 
                  width: 6, 
                  height: 6, 
                  borderRadius: '50%', 
                  background: statusColor 
                }} 
              />
              <Typography variant="caption" sx={{ color: statusColor }}>
                {isOptimal ? 'Optimal' : 'Warning'}
              </Typography>
            </Box>
          </Box>
        )}
      </Box>
    </Paper>
  );
};

export default SensorCard;
