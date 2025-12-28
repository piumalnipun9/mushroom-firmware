import React, { useState, useEffect, useCallback } from 'react';
import { Box, Typography, Paper, IconButton, Chip } from '@mui/material';
import VideocamOffIcon from '@mui/icons-material/VideocamOff';
import RefreshIcon from '@mui/icons-material/Refresh';
import SignalWifiOffIcon from '@mui/icons-material/SignalWifiOff';
import FiberManualRecordIcon from '@mui/icons-material/FiberManualRecord';
import { useThemeMode } from '../../context/ThemeContext';

interface VideoFeedProps {
  streamUrl?: string;
  title?: string;
}

const VideoFeed: React.FC<VideoFeedProps> = ({ 
  streamUrl, 
  title = 'Live Camera Feed' 
}) => {
  const [isConnected, setIsConnected] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>('No video stream available');
  const { mode } = useThemeMode();

  const handleRetryConnection = useCallback(() => {
    setIsLoading(true);
    setError(null);
    
    // Simulate connection attempt
    setTimeout(() => {
      if (streamUrl) {
        setIsConnected(true);
        setError(null);
      } else {
        setIsConnected(false);
        setError('Connection failed - No stream URL configured');
      }
      setIsLoading(false);
    }, 2000);
  }, [streamUrl]);

  useEffect(() => {
    if (streamUrl) {
      handleRetryConnection();
    }
  }, [streamUrl, handleRetryConnection]);

  const borderColor = mode === 'dark' ? 'rgba(255,255,255,0.1)' : 'rgba(0,0,0,0.1)';
  const textColor = mode === 'dark' ? 'rgba(255,255,255,0.7)' : 'rgba(0,0,0,0.6)';

  return (
    <Paper
      elevation={3}
      sx={{
        p: 2,
        height: '100%',
        minHeight: 300,
        background: mode === 'dark' 
          ? 'linear-gradient(135deg, #1a1a2e 0%, #16213e 100%)' 
          : 'linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%)',
        borderRadius: 2,
        border: `1px solid ${borderColor}`,
        display: 'flex',
        flexDirection: 'column'
      }}
    >
      {/* Header */}
      <Box sx={{ 
        display: 'flex', 
        justifyContent: 'space-between', 
        alignItems: 'center', 
        mb: 2 
      }}>
        <Typography 
          variant="h6" 
          sx={{ 
            color: mode === 'dark' ? 'white' : '#1a1a2e',
            fontWeight: 600,
            display: 'flex',
            alignItems: 'center',
            gap: 1
          }}
        >
          {title}
        </Typography>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
          <Chip
            icon={<FiberManualRecordIcon sx={{ fontSize: 12 }} />}
            label={isConnected ? 'LIVE' : 'OFFLINE'}
            size="small"
            sx={{
              backgroundColor: isConnected 
                ? 'rgba(76, 175, 80, 0.2)' 
                : 'rgba(244, 67, 54, 0.2)',
              color: isConnected ? '#4caf50' : '#f44336',
              fontWeight: 600,
              fontSize: '0.7rem',
              '& .MuiChip-icon': {
                color: isConnected ? '#4caf50' : '#f44336'
              }
            }}
          />
          <IconButton 
            size="small" 
            onClick={handleRetryConnection}
            disabled={isLoading}
            sx={{ 
              color: textColor,
              '&:hover': {
                backgroundColor: mode === 'dark' 
                  ? 'rgba(255,255,255,0.1)' 
                  : 'rgba(0,0,0,0.05)'
              }
            }}
          >
            <RefreshIcon 
              sx={{ 
                animation: isLoading ? 'spin 1s linear infinite' : 'none',
                '@keyframes spin': {
                  '0%': { transform: 'rotate(0deg)' },
                  '100%': { transform: 'rotate(360deg)' }
                }
              }} 
            />
          </IconButton>
        </Box>
      </Box>

      {/* Video Container */}
      <Box
        sx={{
          flex: 1,
          borderRadius: 2,
          overflow: 'hidden',
          backgroundColor: mode === 'dark' ? '#0a0a14' : '#e0e0e0',
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          justifyContent: 'center',
          position: 'relative',
          minHeight: 200
        }}
      >
        {isConnected && streamUrl ? (
          <img 
            src={streamUrl} 
            alt="Live Feed"
            style={{
              width: '100%',
              height: '100%',
              objectFit: 'cover'
            }}
          />
        ) : (
          <Box
            sx={{
              display: 'flex',
              flexDirection: 'column',
              alignItems: 'center',
              justifyContent: 'center',
              gap: 2,
              p: 3,
              textAlign: 'center'
            }}
          >
            <Box
              sx={{
                width: 80,
                height: 80,
                borderRadius: '50%',
                backgroundColor: mode === 'dark' 
                  ? 'rgba(244, 67, 54, 0.1)' 
                  : 'rgba(244, 67, 54, 0.1)',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center'
              }}
            >
              {error?.includes('Connection') ? (
                <SignalWifiOffIcon sx={{ fontSize: 40, color: '#f44336' }} />
              ) : (
                <VideocamOffIcon sx={{ fontSize: 40, color: '#f44336' }} />
              )}
            </Box>
            <Box>
              <Typography 
                variant="h6" 
                sx={{ 
                  color: mode === 'dark' ? 'rgba(255,255,255,0.9)' : 'rgba(0,0,0,0.8)',
                  fontWeight: 600,
                  mb: 0.5
                }}
              >
                No Video Signal
              </Typography>
              <Typography 
                variant="body2" 
                sx={{ 
                  color: textColor,
                  maxWidth: 280
                }}
              >
                {error || 'Camera feed is currently unavailable. Check ESP32-CAM connection.'}
              </Typography>
            </Box>
            <Box
              sx={{
                mt: 1,
                px: 2,
                py: 1,
                borderRadius: 1,
                backgroundColor: mode === 'dark' 
                  ? 'rgba(255,255,255,0.05)' 
                  : 'rgba(0,0,0,0.05)',
                border: `1px dashed ${mode === 'dark' ? 'rgba(255,255,255,0.2)' : 'rgba(0,0,0,0.2)'}`
              }}
            >
              <Typography variant="caption" sx={{ color: textColor }}>
                Expected: ESP32-CAM Stream URL
              </Typography>
            </Box>
          </Box>
        )}

        {/* Overlay indicators */}
        {isLoading && (
          <Box
            sx={{
              position: 'absolute',
              top: 0,
              left: 0,
              right: 0,
              bottom: 0,
              backgroundColor: 'rgba(0,0,0,0.7)',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center'
            }}
          >
            <Typography sx={{ color: 'white' }}>
              Attempting to connect...
            </Typography>
          </Box>
        )}
      </Box>

      {/* Footer Info */}
      <Box sx={{ 
        mt: 2, 
        display: 'flex', 
        justifyContent: 'space-between',
        alignItems: 'center'
      }}>
        <Typography variant="caption" sx={{ color: textColor }}>
          Resolution: 640x480 | Format: MJPEG
        </Typography>
        <Typography variant="caption" sx={{ color: textColor }}>
          Last attempt: {new Date().toLocaleTimeString()}
        </Typography>
      </Box>
    </Paper>
  );
};

export default VideoFeed;
