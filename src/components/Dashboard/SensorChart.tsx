import React from 'react';
import {
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  Area,
  AreaChart
} from 'recharts';
import { Paper, Typography, Box, useTheme } from '@mui/material';
import { SensorData } from '../../types';

interface SensorChartProps {
  title: string;
  data: SensorData[];
  dataKey: string;
  color: string;
  unit: string;
  minValue?: number;
  maxValue?: number;
}

interface FormattedDataItem {
  timestamp: number;
  value: number;
  time: string;
  [key: string]: string | number;
}

const SensorChart: React.FC<SensorChartProps> = ({
  title,
  data,
  dataKey,
  color,
  unit,
  minValue,
  maxValue
}) => {
  const theme = useTheme();
  const formattedData: FormattedDataItem[] = data.map((item) => ({
    ...item,
    time: new Date(item.timestamp).toLocaleTimeString(),
    [dataKey]: item.value
  }));

  const lastValue = formattedData.length > 0 ? formattedData[formattedData.length - 1].value : 0;
  const gridColor = theme.palette.mode === 'dark' ? 'rgba(255,255,255,0.1)' : 'rgba(0,0,0,0.1)';
  const axisColor = theme.palette.text.secondary;

  return (
    <Paper 
      elevation={2} 
      sx={{ 
        p: 2, 
        height: '100%',
        borderRadius: 2
      }}
    >
      <Typography 
        variant="h6" 
        gutterBottom 
        sx={{ 
          color: color,
          fontWeight: 600,
          display: 'flex',
          alignItems: 'center',
          gap: 1
        }}
      >
        {title}
        <Typography 
          component="span" 
          color="text.secondary"
          sx={{ 
            fontSize: '0.8rem', 
            fontWeight: 400 
          }}
        >
          ({unit})
        </Typography>
      </Typography>
      <Box sx={{ width: '100%', height: 250 }}>
        <ResponsiveContainer>
          <AreaChart data={formattedData}>
            <defs>
              <linearGradient id={`gradient-${dataKey}`} x1="0" y1="0" x2="0" y2="1">
                <stop offset="5%" stopColor={color} stopOpacity={0.3} />
                <stop offset="95%" stopColor={color} stopOpacity={0} />
              </linearGradient>
            </defs>
            <CartesianGrid strokeDasharray="3 3" stroke={gridColor} />
            <XAxis 
              dataKey="time" 
              stroke={axisColor}
              tick={{ fill: axisColor, fontSize: 10 }}
            />
            <YAxis 
              domain={[minValue || 'auto', maxValue || 'auto']}
              stroke={axisColor}
              tick={{ fill: axisColor, fontSize: 10 }}
            />
            <Tooltip 
              contentStyle={{ 
                backgroundColor: theme.palette.background.paper, 
                border: `1px solid ${color}`,
                borderRadius: 8,
                color: theme.palette.text.primary
              }}
              labelStyle={{ color: theme.palette.text.primary }}
              itemStyle={{ color: color }}
            />
            <Area
              type="monotone"
              dataKey={dataKey}
              stroke={color}
              strokeWidth={2}
              fill={`url(#gradient-${dataKey})`}
            />
          </AreaChart>
        </ResponsiveContainer>
      </Box>
      {formattedData.length > 0 && (
        <Box sx={{ mt: 1, display: 'flex', justifyContent: 'space-between' }}>
          <Typography variant="body2" color="text.secondary">
            Current: <strong style={{ color }}>{lastValue.toFixed(2)} {unit}</strong>
          </Typography>
          <Typography variant="body2" color="text.secondary">
            {formattedData.length} readings
          </Typography>
        </Box>
      )}
    </Paper>
  );
};

export default SensorChart;
