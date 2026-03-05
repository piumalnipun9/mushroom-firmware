import React, { useState, useEffect, useRef, useCallback } from 'react';
import { Box, Typography, Paper, IconButton, Chip } from '@mui/material';
import VideocamOffIcon from '@mui/icons-material/VideocamOff';
import RefreshIcon from '@mui/icons-material/Refresh';
import SignalWifiOffIcon from '@mui/icons-material/SignalWifiOff';
import FiberManualRecordIcon from '@mui/icons-material/FiberManualRecord';
import { useThemeMode } from '../../context/ThemeContext';

interface VideoFeedProps {
  /** Raw base64 JPEG string (no data URI prefix) pushed by ESP32-CAM via Firebase */
  base64Frame?: string;
  /** MJPEG stream URL — used as fallback when no base64 frame is available */
  streamUrl?: string;
  title?: string;
}

/** How many ms without a new frame before we consider the feed stale/offline */
const STALE_TIMEOUT_MS = 8000;

const VideoFeed: React.FC<VideoFeedProps> = ({
  base64Frame,
  streamUrl,
  title = 'Live Camera Feed'
}) => {
  const { mode } = useThemeMode();

  // --- derived image source ---
  // Prefer base64 frame; fall back to MJPEG stream URL
  const [imgSrc, setImgSrc] = useState<string | null>(null);
  const [isLive, setIsLive] = useState(false);
  const [fps, setFps] = useState<number>(0);
  const [useStream, setUseStream] = useState(false);

  // Track FPS from incoming base64 frames
  const frameCountRef = useRef(0);
  const lastFpsTickRef = useRef(Date.now());
  const staleTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const resetStaleTimer = useCallback(() => {
    if (staleTimerRef.current) clearTimeout(staleTimerRef.current);
    staleTimerRef.current = setTimeout(() => {
      setIsLive(false);
    }, STALE_TIMEOUT_MS);
  }, []);

  // --- Handle incoming Base64 frames from Firebase subscription ---
  useEffect(() => {
    if (!base64Frame) return;

    // Build data URI (handle case where firmware already includes the prefix)
    const src = base64Frame.startsWith('data:')
      ? base64Frame
      : `data:image/jpeg;base64,${base64Frame}`;

    setImgSrc(src);
    setIsLive(true);
    setUseStream(false);
    resetStaleTimer();

    // FPS calculation — count frames per second
    frameCountRef.current += 1;
    const now = Date.now();
    const elapsed = now - lastFpsTickRef.current;
    if (elapsed >= 1000) {
      setFps(Math.round((frameCountRef.current * 1000) / elapsed));
      frameCountRef.current = 0;
      lastFpsTickRef.current = now;
    }
  }, [base64Frame, resetStaleTimer]);

  // --- Fallback: switch to MJPEG stream when no base64 data is coming ---
  useEffect(() => {
    if (!base64Frame && streamUrl) {
      setImgSrc(streamUrl);
      setIsLive(true);
      setUseStream(true);
      resetStaleTimer();
    }
  }, [base64Frame, streamUrl, resetStaleTimer]);

  // Cleanup stale timer on unmount
  useEffect(() => () => {
    if (staleTimerRef.current) clearTimeout(staleTimerRef.current);
  }, []);

  const hasSource = !!imgSrc;

  const borderColor = mode === 'dark' ? 'rgba(255,255,255,0.1)' : 'rgba(0,0,0,0.1)';
  const textColor  = mode === 'dark' ? 'rgba(255,255,255,0.7)' : 'rgba(0,0,0,0.6)';

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
      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2 }}>
        <Typography
          variant="h6"
          sx={{ color: mode === 'dark' ? 'white' : '#1a1a2e', fontWeight: 600 }}
        >
          {title}
        </Typography>

        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
          {/* Source badge */}
          {hasSource && isLive && (
            <Chip
              label={useStream ? 'STREAM' : `BASE64 · ${fps} fps`}
              size="small"
              sx={{
                backgroundColor: 'rgba(33,150,243,0.15)',
                color: '#2196f3',
                fontSize: '0.65rem',
                fontWeight: 600,
                height: 20
              }}
            />
          )}

          {/* Live / Offline badge */}
          <Chip
            icon={<FiberManualRecordIcon sx={{ fontSize: 12 }} />}
            label={isLive ? 'LIVE' : 'OFFLINE'}
            size="small"
            sx={{
              backgroundColor: isLive ? 'rgba(76,175,80,0.2)' : 'rgba(244,67,54,0.2)',
              color: isLive ? '#4caf50' : '#f44336',
              fontWeight: 600,
              fontSize: '0.7rem',
              '& .MuiChip-icon': { color: isLive ? '#4caf50' : '#f44336' }
            }}
          />

          {/* Manual refresh (forces stream reload) */}
          {useStream && (
            <IconButton
              size="small"
              onClick={() => { setImgSrc(null); setTimeout(() => setImgSrc(streamUrl!), 100); }}
              sx={{
                color: textColor,
                '&:hover': { backgroundColor: mode === 'dark' ? 'rgba(255,255,255,0.1)' : 'rgba(0,0,0,0.05)' }
              }}
            >
              <RefreshIcon />
            </IconButton>
          )}
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
          alignItems: 'center',
          justifyContent: 'center',
          position: 'relative',
          minHeight: 200
        }}
      >
        {hasSource && isLive ? (
          <img
            src={imgSrc!}
            alt="Live Feed"
            style={{ width: '100%', height: '100%', objectFit: 'cover', display: 'block' }}
          />
        ) : (
          /* ---- Offline / no-signal placeholder ---- */
          <Box
            sx={{
              display: 'flex', flexDirection: 'column', alignItems: 'center',
              justifyContent: 'center', gap: 2, p: 3, textAlign: 'center'
            }}
          >
            <Box
              sx={{
                width: 80, height: 80, borderRadius: '50%',
                backgroundColor: 'rgba(244,67,54,0.1)',
                display: 'flex', alignItems: 'center', justifyContent: 'center'
              }}
            >
              {streamUrl
                ? <SignalWifiOffIcon sx={{ fontSize: 40, color: '#f44336' }} />
                : <VideocamOffIcon  sx={{ fontSize: 40, color: '#f44336' }} />}
            </Box>

            <Box>
              <Typography
                variant="h6"
                sx={{ color: mode === 'dark' ? 'rgba(255,255,255,0.9)' : 'rgba(0,0,0,0.8)', fontWeight: 600, mb: 0.5 }}
              >
                No Video Signal
              </Typography>
              <Typography variant="body2" sx={{ color: textColor, maxWidth: 280 }}>
                {streamUrl
                  ? 'Stream URL found but feed is unavailable. Check ESP32-CAM.'
                  : 'Waiting for ESP32-CAM to push frames to Firebase…'}
              </Typography>
            </Box>

            <Box
              sx={{
                mt: 1, px: 2, py: 1, borderRadius: 1,
                backgroundColor: mode === 'dark' ? 'rgba(255,255,255,0.05)' : 'rgba(0,0,0,0.05)',
                border: `1px dashed ${mode === 'dark' ? 'rgba(255,255,255,0.2)' : 'rgba(0,0,0,0.2)'}`
              }}
            >
              <Typography variant="caption" sx={{ color: textColor }}>
                Source: camera/frame (Base64) · Fallback: stream URL
              </Typography>
            </Box>
          </Box>
        )}
      </Box>

      {/* Footer */}
      <Box sx={{ mt: 2, display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <Typography variant="caption" sx={{ color: textColor }}>
          {useStream ? 'Format: MJPEG Stream' : 'Format: JPEG · Base64 · Firebase RT'}
        </Typography>
        <Typography variant="caption" sx={{ color: textColor }}>
          {isLive ? `Updated: ${new Date().toLocaleTimeString()}` : 'Waiting for signal…'}
        </Typography>
      </Box>
    </Paper>
  );
};

export default VideoFeed;

