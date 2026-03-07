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
import HomeIcon from '@mui/icons-material/Home';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import { RobotArmStatus } from '../../types';
import {
  subscribeRobotArmStatus,
  sendRobotCommand
} from '../../services/firebaseService';

interface Plot {
  id: number;
  name: string;
  status: 'active' | 'inactive';
}

const RobotArmControl: React.FC = () => {
  const [robotStatus, setRobotStatus] = useState<RobotArmStatus>({
    currentPlot: 0,
    state: 'idle',
    lastAction: 'Waiting for data...'
  });
  const [plots] = useState<Plot[]>([
    { id: 0, name: 'Home', status: 'active' },
    { id: 1, name: 'Plot 1', status: 'active' },
    { id: 2, name: 'Plot 2', status: 'active' },
    { id: 3, name: 'Plot 3', status: 'active' },
    { id: 4, name: 'Plot 4', status: 'active' },
  ]);
  const [selectedPlot, setSelectedPlot] = useState<number>(1);
  const isMoving = robotStatus.state === 'moving' || robotStatus.state === 'homing';

  useEffect(() => {
    const unsub = subscribeRobotArmStatus((data) => {
      if (data) {
        setRobotStatus({
          currentPlot: data.currentPlot ?? 0,
          state: data.state || 'idle',
          lastAction: data.lastAction || 'System ready'
        });
      }
    });
    return () => unsub();
  }, []);

  const handlePlotSelect = (event: SelectChangeEvent<number>) => {
    setSelectedPlot(event.target.value as number);
  };

  const handleMoveToPlot = async () => {
    await sendRobotCommand('move', selectedPlot);
  };

  const handleGoHome = async () => {
    await sendRobotCommand('home');
  };



  const getStatusColor = (status: string) => {
    switch (status) {
      case 'idle': return '#4caf50';
      case 'moving': return '#ff9800';
      case 'homing': return '#9c27b0';
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
                <LocationOnIcon sx={{ color: 'primary.main', fontSize: 32, position: 'absolute', display: robotStatus.currentPlot === 0 ? 'none' : 'block' }} />
                <HomeIcon sx={{ color: 'primary.main', fontSize: 32, position: 'absolute', display: robotStatus.currentPlot === 0 ? 'block' : 'none' }} />
              </Box>
              <Box>
                <Typography variant="body2" color="text.secondary">
                  Current Position
                </Typography>
                <Typography variant="h5" color="text.primary" sx={{ fontWeight: 600 }}>
                  {robotStatus.currentPlot === 0 ? 'Home' : `Plot ${robotStatus.currentPlot}`}
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
                  label={robotStatus.state.toUpperCase()}
                  sx={{
                    backgroundColor: `${getStatusColor(robotStatus.state)}20`,
                    color: getStatusColor(robotStatus.state),
                    fontWeight: 600,
                    fontSize: '0.9rem',
                    py: 2
                  }}
                  icon={
                    isMoving ? (
                      <CircularProgress size={16} sx={{ color: '#ff9800' }} />
                    ) : (
                      <CheckCircleIcon sx={{ color: `${getStatusColor(robotStatus.state)} !important` }} />
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
                {robotStatus.lastAction}
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
                      {plot.id === robotStatus.currentPlot && (
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
                disabled={isMoving || selectedPlot === robotStatus.currentPlot || selectedPlot === 0}
                color="success"
                sx={{ flex: 1 }}
              >
                {isMoving && robotStatus.state === 'moving' ? 'Moving...' : 'Move to Plot'}
              </Button>

              <Button
                variant="contained"
                size="large"
                startIcon={<HomeIcon />}
                onClick={handleGoHome}
                disabled={isMoving || robotStatus.currentPlot === 0}
                color="primary"
              >
                Go Home
              </Button>
            </Box>

          </Box>
      </Box>
    </Paper>

        {/* Plot Grid Visualization */ }
  <Paper elevation={2} sx={{ p: 3, borderRadius: 2, gridColumn: { xs: '1', lg: '1 / -1' } }}>
    <Typography variant="h6" color="text.primary" sx={{ fontWeight: 600, mb: 3 }}>
      Plot Overview
    </Typography>

    <Box
      sx={{
        display: 'grid',
        gridTemplateColumns: 'repeat(2, 1fr)',
        gap: 2
      }}
    >
      {plots.map((plot) => (
        <Paper
          key={plot.id}
          variant="outlined"
          onClick={() => {
            // Home (id=0) is display-only; use Go Home button
            if (plot.status === 'active' && !isMoving && plot.id !== 0) {
              setSelectedPlot(plot.id);
            }
          }}
          sx={{
            p: 2,
            backgroundColor:
              robotStatus.currentPlot === plot.id
                ? 'success.main'
                : selectedPlot === plot.id
                  ? 'primary.main'
                  : 'action.hover',
            opacity: plot.status === 'inactive' ? 0.5 : 1,
            borderColor:
              robotStatus.currentPlot === plot.id
                ? 'success.main'
                : selectedPlot === plot.id
                  ? 'primary.main'
                  : 'divider',
            borderWidth: 2,
            borderRadius: 2,
            cursor: plot.status === 'active' && !isMoving && plot.id !== 0 ? 'pointer' : 'default',
            transition: 'all 0.2s',
            '&:hover': plot.status === 'active' && !isMoving && plot.id !== 0 ? {
              transform: 'scale(1.02)'
            } : {}
          }}
        >
          <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <Typography
              variant="h6"
              sx={{
                fontWeight: 600,
                color: robotStatus.currentPlot === plot.id || selectedPlot === plot.id ? 'white' : 'text.primary'
              }}
            >
              {plot.name}
            </Typography>
            {robotStatus.currentPlot === plot.id && (
              <PrecisionManufacturingIcon sx={{ color: 'white' }} />
            )}
          </Box>
          <Typography
            variant="caption"
            sx={{
              color: robotStatus.currentPlot === plot.id || selectedPlot === plot.id ? 'rgba(255,255,255,0.8)' : 'text.secondary'
            }}
          >
            {plot.status === 'inactive' ? 'Inactive' : 'Active'}
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
      </Box >
    </Box >
  );
};

export default RobotArmControl;
