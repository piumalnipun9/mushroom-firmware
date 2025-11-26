import React, { useEffect, useState } from 'react';
import {
  Box,
  Typography,
  Paper,
  Chip,
  Button,
  LinearProgress,
  Divider,
  Switch,
  FormControlLabel,
  CircularProgress
} from '@mui/material';
import SmartToyIcon from '@mui/icons-material/SmartToy';
import TrendingUpIcon from '@mui/icons-material/TrendingUp';
import AccessTimeIcon from '@mui/icons-material/AccessTime';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import WarningIcon from '@mui/icons-material/Warning';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import PauseIcon from '@mui/icons-material/Pause';
import RefreshIcon from '@mui/icons-material/Refresh';
import LightbulbIcon from '@mui/icons-material/Lightbulb';
import { MLModelInfo } from '../../types';
import { subscribeMLModelInfo, updateMLModelStatus } from '../../services/firebaseService';

const MLModelInfoComponent: React.FC = () => {
  const [modelInfo, setModelInfo] = useState<MLModelInfo | null>(null);
  const [isAutoMode, setIsAutoMode] = useState(true);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const unsubscribe = subscribeMLModelInfo((data) => {
      if (data) {
        setModelInfo(data);
      }
      setLoading(false);
    });
    
    // Set loading to false after timeout if no data
    const timeout = setTimeout(() => setLoading(false), 3000);
    
    return () => {
      unsubscribe();
      clearTimeout(timeout);
    };
  }, []);

  const handleStatusToggle = async () => {
    if (modelInfo) {
      const newStatus = modelInfo.status === 'active' ? 'inactive' : 'active';
      await updateMLModelStatus(newStatus);
    }
  };

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'active': return '#4caf50';
      case 'inactive': return '#9e9e9e';
      case 'training': return '#ff9800';
      default: return '#9e9e9e';
    }
  };

  if (loading) {
    return (
      <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', minHeight: '50vh', gap: 2 }}>
        <CircularProgress color="secondary" />
        <Typography color="text.secondary">Loading ML model data from Firebase...</Typography>
      </Box>
    );
  }

  if (!modelInfo) {
    return (
      <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', minHeight: '50vh', gap: 2 }}>
        <SmartToyIcon sx={{ fontSize: 60, color: 'text.disabled' }} />
        <Typography variant="h6" color="text.secondary">No ML Model Data</Typography>
        <Typography color="text.secondary" sx={{ textAlign: 'center' }}>
          Initialize data from the Dashboard to see ML model information.
        </Typography>
      </Box>
    );
  }

  return (
    <Box>
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 2, mb: 3 }}>
        <SmartToyIcon sx={{ fontSize: 40, color: 'secondary.main' }} />
        <Typography 
          variant="h4" 
          color="text.primary"
          sx={{ fontWeight: 700 }}
        >
          ML Model Information
        </Typography>
      </Box>

      <Box 
        sx={{ 
          display: 'grid', 
          gridTemplateColumns: { xs: '1fr', lg: 'repeat(2, 1fr)' },
          gap: 3
        }}
      >
        {/* Model Overview */}
        <Paper elevation={2} sx={{ p: 3, borderRadius: 2 }}>
          <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', mb: 3 }}>
            <Box>
              <Typography variant="h5" color="text.primary" sx={{ fontWeight: 600 }}>
                {modelInfo.name}
              </Typography>
              <Typography variant="body2" color="text.secondary" sx={{ mt: 0.5 }}>
                Version {modelInfo.version}
              </Typography>
            </Box>
            <Chip
              label={modelInfo.status.toUpperCase()}
              sx={{
                backgroundColor: `${getStatusColor(modelInfo.status)}20`,
                color: getStatusColor(modelInfo.status),
                fontWeight: 600
              }}
            />
          </Box>

          <Typography variant="body2" color="text.secondary" sx={{ mb: 3 }}>
            {modelInfo.description}
          </Typography>

          <Box sx={{ mb: 3 }}>
            <Typography variant="body2" color="text.secondary" sx={{ mb: 1 }}>
              Model Accuracy
            </Typography>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
              <Box sx={{ flex: 1 }}>
                <LinearProgress
                  variant="determinate"
                  value={modelInfo.accuracy}
                  color="success"
                  sx={{ height: 10, borderRadius: 5 }}
                />
              </Box>
              <Typography variant="h6" sx={{ color: '#4caf50', fontWeight: 600, minWidth: 60 }}>
                {modelInfo.accuracy}%
              </Typography>
            </Box>
          </Box>

          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, color: 'text.secondary' }}>
            <AccessTimeIcon fontSize="small" />
            <Typography variant="body2">
              Last trained: {new Date(modelInfo.lastTrainedDate).toLocaleDateString()}
            </Typography>
          </Box>

          <Divider sx={{ my: 3 }} />

          <Typography variant="subtitle2" color="text.secondary" sx={{ mb: 2 }}>
            Input Features
          </Typography>
          <Box sx={{ display: 'flex', flexWrap: 'wrap', gap: 1 }}>
            {modelInfo.features.map((feature, index) => (
              <Chip
                key={index}
                label={feature}
                size="small"
                color="primary"
                variant="outlined"
                sx={{ fontSize: '0.75rem' }}
              />
            ))}
          </Box>
        </Paper>

        {/* Model Predictions */}
        <Paper elevation={2} sx={{ p: 3, borderRadius: 2 }}>
          <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600, mb: 3, display: 'flex', alignItems: 'center', gap: 1 }}>
            <TrendingUpIcon color="success" />
            Current Predictions
          </Typography>

          {/* Fruiting Readiness */}
          <Box sx={{ mb: 3 }}>
            <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 1 }}>
              <Typography variant="body2" color="text.secondary">
                Fruiting Readiness
              </Typography>
              <Typography variant="body2" sx={{ color: '#fbbf24', fontWeight: 600 }}>
                {modelInfo.predictions.fruitingReadiness}%
              </Typography>
            </Box>
            <LinearProgress
              variant="determinate"
              value={modelInfo.predictions.fruitingReadiness}
              sx={{
                height: 8,
                borderRadius: 4,
                '& .MuiLinearProgress-bar': {
                  backgroundColor: '#fbbf24',
                  borderRadius: 4
                }
              }}
            />
          </Box>

          {/* Health Score */}
          <Box sx={{ mb: 3 }}>
            <Box sx={{ display: 'flex', justifyContent: 'space-between', mb: 1 }}>
              <Typography variant="body2" color="text.secondary">
                Health Score
              </Typography>
              <Typography variant="body2" sx={{ color: '#4caf50', fontWeight: 600 }}>
                {modelInfo.predictions.healthScore}%
              </Typography>
            </Box>
            <LinearProgress
              variant="determinate"
              value={modelInfo.predictions.healthScore}
              color="success"
              sx={{ height: 8, borderRadius: 4 }}
            />
          </Box>

          {/* Estimated Harvest */}
          <Paper
            variant="outlined"
            sx={{
              p: 2,
              borderRadius: 2,
              borderColor: 'success.main',
              bgcolor: 'action.hover'
            }}
          >
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
              <CheckCircleIcon sx={{ color: 'success.main', fontSize: 32 }} />
              <Box>
                <Typography variant="body2" color="text.secondary">
                  Estimated Harvest Date
                </Typography>
                <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600 }}>
                  {new Date(modelInfo.predictions.estimatedHarvestDate).toLocaleDateString('en-US', {
                    weekday: 'long',
                    year: 'numeric',
                    month: 'long',
                    day: 'numeric'
                  })}
                </Typography>
              </Box>
            </Box>
          </Paper>

          <Divider sx={{ my: 3 }} />

          {/* Model Controls */}
          <Typography variant="subtitle2" color="text.secondary" sx={{ mb: 2 }}>
            Model Controls
          </Typography>

          <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            <FormControlLabel
              control={
                <Switch
                  checked={isAutoMode}
                  onChange={(e) => setIsAutoMode(e.target.checked)}
                  color="secondary"
                />
              }
              label={
                <Typography color="text.secondary">
                  Auto Light Control (75% intensity)
                </Typography>
              }
            />

            <Box sx={{ display: 'flex', gap: 2 }}>
              <Button
                variant="contained"
                startIcon={modelInfo.status === 'active' ? <PauseIcon /> : <PlayArrowIcon />}
                onClick={handleStatusToggle}
                color={modelInfo.status === 'active' ? 'warning' : 'success'}
              >
                {modelInfo.status === 'active' ? 'Pause Model' : 'Activate Model'}
              </Button>
              <Button
                variant="outlined"
                startIcon={<RefreshIcon />}
                color="secondary"
              >
                Retrain Model
              </Button>
            </Box>
          </Box>
        </Paper>

        {/* Alerts & Insights */}
        <Paper
          elevation={2}
          sx={{
            p: 3,
            borderRadius: 2,
            gridColumn: { xs: '1', lg: '1 / -1' }
          }}
        >
          <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600, mb: 3, display: 'flex', alignItems: 'center', gap: 1 }}>
            <WarningIcon sx={{ color: '#fbbf24' }} />
            Alerts & Recommendations
          </Typography>

          <Box sx={{ display: 'grid', gridTemplateColumns: { xs: '1fr', md: 'repeat(3, 1fr)' }, gap: 2 }}>
            <Paper
              variant="outlined"
              sx={{
                p: 2,
                borderRadius: 2,
                borderColor: 'success.main',
                bgcolor: 'action.hover'
              }}
            >
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 1 }}>
                <CheckCircleIcon sx={{ color: 'success.main', fontSize: 18 }} />
                <Typography variant="body2" sx={{ color: 'success.main', fontWeight: 600 }}>
                  Optimal Conditions
                </Typography>
              </Box>
              <Typography variant="body2" color="text.secondary">
                Temperature and humidity are within optimal range for fruiting.
              </Typography>
            </Paper>

            <Paper
              variant="outlined"
              sx={{
                p: 2,
                borderRadius: 2,
                borderColor: 'warning.main',
                bgcolor: 'action.hover'
              }}
            >
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 1 }}>
                <WarningIcon sx={{ color: 'warning.main', fontSize: 18 }} />
                <Typography variant="body2" sx={{ color: 'warning.main', fontWeight: 600 }}>
                  CO2 Warning
                </Typography>
              </Box>
              <Typography variant="body2" color="text.secondary">
                CO2 levels approaching upper limit. Consider increasing ventilation.
              </Typography>
            </Paper>

            <Paper
              variant="outlined"
              sx={{
                p: 2,
                borderRadius: 2,
                borderColor: 'info.main',
                bgcolor: 'action.hover'
              }}
            >
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 1 }}>
                <LightbulbIcon sx={{ color: 'info.main', fontSize: 18 }} />
                <Typography variant="body2" sx={{ color: 'info.main', fontWeight: 600 }}>
                  Suggestion
                </Typography>
              </Box>
              <Typography variant="body2" color="text.secondary">
                Based on current data, harvest window will open in approximately 9 days.
              </Typography>
            </Paper>
          </Box>
        </Paper>
      </Box>
    </Box>
  );
};

export default MLModelInfoComponent;
