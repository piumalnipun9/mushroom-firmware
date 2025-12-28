import React, { useEffect, useState } from 'react';
import {
  Box,
  Typography,
  Paper,
  Button,
  Chip,
  Select,
  MenuItem,
  FormControl,
  InputLabel,
  CircularProgress,
  Divider,
  SelectChangeEvent
} from '@mui/material';
import PrecisionManufacturingIcon from '@mui/icons-material/PrecisionManufacturing';
import LocationOnIcon from '@mui/icons-material/LocationOn';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import StopIcon from '@mui/icons-material/Stop';
import HomeIcon from '@mui/icons-material/Home';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import { RobotArmPosition, Plot } from '../../types';
import { 
  subscribeRobotArmPosition, 
  subscribePlots, 
  moveRobotToPlot, 
  updateRobotStatus
} from '../../services/firebaseService';

const RobotArmControl: React.FC = () => {
  const [robotPosition, setRobotPosition] = useState<RobotArmPosition>({
    currentPlot: 1,
    status: 'idle',
    lastAction: 'Waiting for data...'
  });
  const [plots, setPlots] = useState<Plot[]>([]);
  const [selectedPlot, setSelectedPlot] = useState<number>(1);
  const [isMoving, setIsMoving] = useState(false);

  useEffect(() => {
    const unsubscribeRobot = subscribeRobotArmPosition((data) => {
      if (data) {
        setRobotPosition({
          currentPlot: data.currentPlot || 1,
          status: data.status || 'idle',
          lastAction: data.lastAction || 'System ready'
        });
        setIsMoving(data.status === 'moving');
      }
    });

    const unsubscribePlots = subscribePlots((data) => {
      if (data && data.length > 0) {
        setPlots(data);
      } else {
        // Default plots if none in Firebase
        setPlots([
          { id: 1, name: 'Plot 1', status: 'active', lastVisited: new Date().toISOString() },
          { id: 2, name: 'Plot 2', status: 'active', lastVisited: new Date().toISOString() },
          { id: 3, name: 'Plot 3', status: 'active', lastVisited: new Date().toISOString() },
          { id: 4, name: 'Plot 4', status: 'active', lastVisited: new Date().toISOString() },
          { id: 5, name: 'Plot 5', status: 'inactive', lastVisited: new Date().toISOString() },
          { id: 6, name: 'Plot 6', status: 'active', lastVisited: new Date().toISOString() },
        ]);
      }
    });

    return () => {
      unsubscribeRobot();
      unsubscribePlots();
    };
  }, []);

  const handlePlotSelect = (event: SelectChangeEvent<number>) => {
    setSelectedPlot(event.target.value as number);
  };

  const handleMoveToPlot = async () => {
    setIsMoving(true);
    await moveRobotToPlot(selectedPlot);
    
    // Simulate movement time
    setTimeout(() => {
      setRobotPosition(prev => ({
        ...prev,
        currentPlot: selectedPlot,
        status: 'idle',
        lastAction: `Arrived at Plot ${selectedPlot}`
      }));
      setIsMoving(false);
    }, 3000);
  };

  const handleEmergencyStop = async () => {
    await updateRobotStatus('idle');
    setIsMoving(false);
    setRobotPosition(prev => ({
      ...prev,
      status: 'idle',
      lastAction: 'Emergency stop activated'
    }));
  };

  const handleReturnHome = async () => {
    setIsMoving(true);
    await moveRobotToPlot(1);
    
    setTimeout(() => {
      setRobotPosition(prev => ({
        ...prev,
        currentPlot: 1,
        status: 'idle',
        lastAction: 'Returned to home position'
      }));
      setIsMoving(false);
    }, 2000);
  };

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'idle': return '#4caf50';
      case 'moving': return '#ff9800';
      case 'operating': return '#2196f3';
      default: return '#9e9e9e';
    }
  };

  return (
    <Box>
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 2, mb: 3 }}>
        <PrecisionManufacturingIcon sx={{ fontSize: 40 }} color="primary" />
        <Typography 
          variant="h4" 
          color="text.primary"
          sx={{ fontWeight: 700 }}
        >
          Robot Arm Control
        </Typography>
      </Box>

      <Box 
        sx={{ 
          display: 'grid', 
          gridTemplateColumns: { xs: '1fr', lg: 'repeat(2, 1fr)' },
          gap: 3
        }}
      >
        {/* Robot Status Panel */}
        <Paper elevation={2} sx={{ p: 3, borderRadius: 2 }}>
          <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600, mb: 3 }}>
            Robot Status
          </Typography>

          <Box sx={{ display: 'flex', flexDirection: 'column', gap: 3 }}>
            {/* Current Position */}
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
              <Box
                sx={{
                  width: 60,
                  height: 60,
                  borderRadius: 2,
                  bgcolor: 'primary.main',
                  opacity: 0.1,
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                  position: 'relative'
                }}
              >
                <LocationOnIcon sx={{ color: 'primary.main', fontSize: 32, position: 'absolute' }} />
              </Box>
              <Box>
                <Typography variant="body2" color="text.secondary">
                  Current Position
                </Typography>
                <Typography variant="h5" color="text.primary" sx={{ fontWeight: 600 }}>
                  Plot {robotPosition.currentPlot}
                </Typography>
              </Box>
            </Box>

            {/* Status Indicator */}
            <Box>
              <Typography variant="body2" color="text.secondary" sx={{ mb: 1 }}>
                Status
              </Typography>
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                <Chip
                  label={robotPosition.status.toUpperCase()}
                  sx={{
                    backgroundColor: `${getStatusColor(robotPosition.status)}20`,
                    color: getStatusColor(robotPosition.status),
                    fontWeight: 600,
                    fontSize: '0.9rem',
                    py: 2
                  }}
                  icon={
                    isMoving ? (
                      <CircularProgress size={16} sx={{ color: '#ff9800' }} />
                    ) : (
                      <CheckCircleIcon sx={{ color: `${getStatusColor(robotPosition.status)} !important` }} />
                    )
                  }
                />
              </Box>
            </Box>

            {/* Last Action */}
            <Box>
              <Typography variant="body2" color="text.secondary" sx={{ mb: 1 }}>
                Last Action
              </Typography>
              <Typography variant="body1" color="text.primary">
                {robotPosition.lastAction}
              </Typography>
            </Box>
          </Box>
        </Paper>

        {/* Movement Control Panel */}
        <Paper elevation={2} sx={{ p: 3, borderRadius: 2 }}>
          <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600, mb: 3 }}>
            Movement Control
          </Typography>

          <Box sx={{ display: 'flex', flexDirection: 'column', gap: 3 }}>
            {/* Plot Selection */}
            <FormControl fullWidth>
              <InputLabel id="plot-select-label">
                Select Target Plot
              </InputLabel>
              <Select
                labelId="plot-select-label"
                value={selectedPlot}
                label="Select Target Plot"
                onChange={handlePlotSelect}
              >
                {plots.map((plot) => (
                  <MenuItem 
                    key={plot.id} 
                    value={plot.id}
                    disabled={plot.status === 'inactive'}
                  >
                    <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                      {plot.name}
                      {plot.status === 'inactive' && (
                        <Chip label="Inactive" size="small" color="warning" />
                      )}
                      {plot.id === robotPosition.currentPlot && (
                        <Chip label="Current" size="small" color="success" />
                      )}
                    </Box>
                  </MenuItem>
                ))}
              </Select>
            </FormControl>

            {/* Control Buttons */}
            <Box sx={{ display: 'flex', gap: 2, flexWrap: 'wrap' }}>
              <Button
                variant="contained"
                size="large"
                startIcon={isMoving ? <CircularProgress size={20} color="inherit" /> : <PlayArrowIcon />}
                onClick={handleMoveToPlot}
                disabled={isMoving || selectedPlot === robotPosition.currentPlot}
                color="success"
                sx={{ flex: 1 }}
              >
                {isMoving ? 'Moving...' : 'Move to Plot'}
              </Button>

              <Button
                variant="contained"
                size="large"
                startIcon={<HomeIcon />}
                onClick={handleReturnHome}
                disabled={isMoving || robotPosition.currentPlot === 1}
                color="primary"
              >
                Home
              </Button>
            </Box>

            <Button
              variant="contained"
              size="large"
              startIcon={<StopIcon />}
              onClick={handleEmergencyStop}
              color="error"
            >
              Emergency Stop
            </Button>
          </Box>
        </Paper>

        {/* Plot Grid Visualization */}
        <Paper elevation={2} sx={{ p: 3, borderRadius: 2, gridColumn: { xs: '1', lg: '1 / -1' } }}>
          <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600, mb: 3 }}>
            Plot Overview
          </Typography>

          <Box 
            sx={{ 
              display: 'grid', 
              gridTemplateColumns: 'repeat(3, 1fr)',
              gap: 2
            }}
          >
            {plots.map((plot) => (
              <Paper
                key={plot.id}
                variant="outlined"
                onClick={() => {
                  if (plot.status === 'active' && !isMoving) {
                    setSelectedPlot(plot.id);
                  }
                }}
                sx={{
                  p: 2,
                  backgroundColor: 
                    robotPosition.currentPlot === plot.id
                      ? 'success.main'
                      : selectedPlot === plot.id
                        ? 'primary.main'
                        : 'action.hover',
                  opacity: plot.status === 'inactive' ? 0.5 : 1,
                  borderColor: 
                    robotPosition.currentPlot === plot.id
                      ? 'success.main'
                      : selectedPlot === plot.id
                        ? 'primary.main'
                        : 'divider',
                  borderWidth: 2,
                  borderRadius: 2,
                  cursor: plot.status === 'active' && !isMoving ? 'pointer' : 'default',
                  transition: 'all 0.2s',
                  '&:hover': plot.status === 'active' && !isMoving ? {
                    transform: 'scale(1.02)'
                  } : {}
                }}
              >
                <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                  <Typography 
                    variant="h6" 
                    sx={{ 
                      fontWeight: 600,
                      color: robotPosition.currentPlot === plot.id || selectedPlot === plot.id ? 'white' : 'text.primary'
                    }}
                  >
                    {plot.name}
                  </Typography>
                  {robotPosition.currentPlot === plot.id && (
                    <PrecisionManufacturingIcon sx={{ color: 'white' }} />
                  )}
                </Box>
                <Typography 
                  variant="caption" 
                  sx={{ 
                    color: robotPosition.currentPlot === plot.id || selectedPlot === plot.id ? 'rgba(255,255,255,0.8)' : 'text.secondary'
                  }}
                >
                  {plot.status === 'inactive' ? 'Inactive' : `Last visited: ${new Date(plot.lastVisited).toLocaleTimeString()}`}
                </Typography>
              </Paper>
            ))}
          </Box>

          <Divider sx={{ my: 3 }} />

          <Box sx={{ display: 'flex', gap: 3, flexWrap: 'wrap' }}>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
              <Box sx={{ width: 16, height: 16, borderRadius: 1, bgcolor: 'success.main' }} />
              <Typography variant="body2" color="text.secondary">Current Position</Typography>
            </Box>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
              <Box sx={{ width: 16, height: 16, borderRadius: 1, bgcolor: 'primary.main' }} />
              <Typography variant="body2" color="text.secondary">Selected Target</Typography>
            </Box>
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
              <Box sx={{ width: 16, height: 16, borderRadius: 1, bgcolor: 'action.disabled' }} />
              <Typography variant="body2" color="text.secondary">Inactive Plot</Typography>
            </Box>
          </Box>
        </Paper>
      </Box>
    </Box>
  );
};

export default RobotArmControl;
