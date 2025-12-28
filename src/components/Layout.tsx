import React, { useState } from 'react';
import {
  Box,
  Drawer,
  AppBar,
  Toolbar,
  List,
  Typography,
  Divider,
  IconButton,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
  useTheme,
  useMediaQuery,
  Avatar,
  Menu,
  MenuItem,
  Tooltip
} from '@mui/material';
import MenuIcon from '@mui/icons-material/Menu';
import DashboardIcon from '@mui/icons-material/Dashboard';
import SmartToyIcon from '@mui/icons-material/SmartToy';
import PrecisionManufacturingIcon from '@mui/icons-material/PrecisionManufacturing';
import SensorsIcon from '@mui/icons-material/Sensors';
import ChevronLeftIcon from '@mui/icons-material/ChevronLeft';
import Brightness4Icon from '@mui/icons-material/Brightness4';
import Brightness7Icon from '@mui/icons-material/Brightness7';
import PersonIcon from '@mui/icons-material/Person';
import SettingsIcon from '@mui/icons-material/Settings';
import LogoutIcon from '@mui/icons-material/Logout';
import AgricultureIcon from '@mui/icons-material/Agriculture';
import CloudDoneIcon from '@mui/icons-material/CloudDone';

import Dashboard from './Dashboard/Dashboard';
import MLModelInfoComponent from './MLModel/MLModelInfo';
import RobotArmControl from './RobotArm/RobotArmControl';
import SensorControls from './Sensors/SensorControls';
import { useThemeMode } from '../context/ThemeContext';

const drawerWidth = 280;

interface TabItem {
  id: string;
  label: string;
  icon: React.ReactNode;
  component: React.ReactNode;
}

// Mock user data
const currentUser = {
  name: 'Admin User',
  email: 'admin@mushroom-farm.com',
  role: 'Administrator',
  avatar: ''
};

const Layout: React.FC = () => {
  const [currentTab, setCurrentTab] = useState('dashboard');
  const [mobileOpen, setMobileOpen] = useState(false);
  const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
  const theme = useTheme();
  const isMobile = useMediaQuery(theme.breakpoints.down('md'));
  const { mode, toggleTheme } = useThemeMode();

  const tabs: TabItem[] = [
    {
      id: 'dashboard',
      label: 'Dashboard',
      icon: <DashboardIcon />,
      component: <Dashboard />
    },
    {
      id: 'ml-model',
      label: 'ML Model',
      icon: <SmartToyIcon />,
      component: <MLModelInfoComponent />
    },
    {
      id: 'robot-arm',
      label: 'Robot Arm Control',
      icon: <PrecisionManufacturingIcon />,
      component: <RobotArmControl />
    },
    {
      id: 'sensors',
      label: 'Sensor Controls',
      icon: <SensorsIcon />,
      component: <SensorControls />
    }
  ];

  const handleDrawerToggle = () => {
    setMobileOpen(!mobileOpen);
  };

  const handleTabChange = (tabId: string) => {
    setCurrentTab(tabId);
    if (isMobile) {
      setMobileOpen(false);
    }
  };

  const handleUserMenuOpen = (event: React.MouseEvent<HTMLElement>) => {
    setAnchorEl(event.currentTarget);
  };

  const handleUserMenuClose = () => {
    setAnchorEl(null);
  };

  const getCurrentComponent = () => {
    const tab = tabs.find(t => t.id === currentTab);
    return tab?.component || <Dashboard />;
  };

  // Theme-aware colors
  const bgSecondary = mode === 'dark' ? '#1a1a2e' : '#ffffff';
  const bgGradient = mode === 'dark' 
    ? 'linear-gradient(180deg, #0f0f1a 0%, #1a1a2e 100%)'
    : 'linear-gradient(180deg, #ffffff 0%, #f8f9fa 100%)';
  const borderColor = mode === 'dark' ? 'rgba(255,255,255,0.1)' : 'rgba(0,0,0,0.1)';
  const textPrimary = mode === 'dark' ? '#ffffff' : '#1a1a2e';
  const textSecondary = mode === 'dark' ? 'rgba(255,255,255,0.7)' : 'rgba(0,0,0,0.6)';
  const textMuted = mode === 'dark' ? 'rgba(255,255,255,0.4)' : 'rgba(0,0,0,0.4)';

  const drawer = (
    <Box sx={{ height: '100%', display: 'flex', flexDirection: 'column', background: bgGradient }}>
      <Toolbar sx={{ 
        borderBottom: `1px solid ${borderColor}`
      }}>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
          <AgricultureIcon sx={{ color: '#4caf50', fontSize: 28 }} />
          <Typography variant="h6" noWrap sx={{ color: textPrimary, fontWeight: 700 }}>
            Mushroom Farm
          </Typography>
        </Box>
        {isMobile && (
          <IconButton onClick={handleDrawerToggle} sx={{ ml: 'auto', color: textSecondary }}>
            <ChevronLeftIcon />
          </IconButton>
        )}
      </Toolbar>
      <Divider sx={{ borderColor }} />
      <List sx={{ flex: 1, py: 2 }}>
        {tabs.map((tab) => (
          <ListItem key={tab.id} disablePadding sx={{ px: 1, mb: 0.5 }}>
            <ListItemButton
              selected={currentTab === tab.id}
              onClick={() => handleTabChange(tab.id)}
              sx={{
                borderRadius: 2,
                '&.Mui-selected': {
                  backgroundColor: mode === 'dark' ? 'rgba(59, 130, 246, 0.2)' : 'rgba(59, 130, 246, 0.1)',
                  '&:hover': {
                    backgroundColor: mode === 'dark' ? 'rgba(59, 130, 246, 0.3)' : 'rgba(59, 130, 246, 0.15)'
                  },
                  '& .MuiListItemIcon-root': {
                    color: '#3b82f6'
                  },
                  '& .MuiListItemText-primary': {
                    color: '#3b82f6',
                    fontWeight: 600
                  }
                },
                '&:hover': {
                  backgroundColor: mode === 'dark' ? 'rgba(255,255,255,0.05)' : 'rgba(0,0,0,0.04)'
                }
              }}
            >
              <ListItemIcon sx={{ color: textSecondary, minWidth: 44 }}>
                {tab.icon}
              </ListItemIcon>
              <ListItemText 
                primary={tab.label} 
                sx={{ 
                  '& .MuiListItemText-primary': { 
                    color: textPrimary,
                    fontSize: '0.95rem'
                  } 
                }}
              />
            </ListItemButton>
          </ListItem>
        ))}
      </List>
      <Divider sx={{ borderColor }} />
      <Box sx={{ p: 2 }}>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 1 }}>
          <CloudDoneIcon sx={{ fontSize: 16, color: '#4caf50' }} />
          <Typography variant="caption" sx={{ color: '#4caf50' }}>
            Connected to Firebase
          </Typography>
        </Box>
        <Typography variant="caption" sx={{ color: textMuted }}>
          Smart Mushroom Cultivation v1.0
        </Typography>
      </Box>
    </Box>
  );

  return (
    <Box sx={{ display: 'flex', minHeight: '100vh' }}>
      {/* Top App Bar */}
      <AppBar
        position="fixed"
        sx={{
          width: { md: `calc(100% - ${drawerWidth}px)` },
          ml: { md: `${drawerWidth}px` },
          background: mode === 'dark' 
            ? 'linear-gradient(135deg, #1a1a2e 0%, #16213e 100%)' 
            : '#ffffff',
          boxShadow: mode === 'dark' ? 'none' : '0 1px 3px rgba(0,0,0,0.1)',
          borderBottom: `1px solid ${borderColor}`
        }}
      >
        <Toolbar sx={{ justifyContent: 'space-between' }}>
          <Box sx={{ display: 'flex', alignItems: 'center' }}>
            {isMobile && (
              <IconButton
                color="inherit"
                edge="start"
                onClick={handleDrawerToggle}
                sx={{ mr: 2, color: textPrimary }}
              >
                <MenuIcon />
              </IconButton>
            )}
            {isMobile && (
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                <AgricultureIcon sx={{ color: '#4caf50' }} />
                <Typography variant="h6" sx={{ color: textPrimary, fontWeight: 700 }}>
                  Mushroom Farm
                </Typography>
              </Box>
            )}
          </Box>

          {/* Right side - Theme toggle and User info */}
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
            {/* Theme Toggle */}
            <Tooltip title={mode === 'dark' ? 'Switch to Light Mode' : 'Switch to Dark Mode'}>
              <IconButton 
                onClick={toggleTheme} 
                sx={{ 
                  color: textSecondary,
                  '&:hover': {
                    backgroundColor: mode === 'dark' ? 'rgba(255,255,255,0.1)' : 'rgba(0,0,0,0.04)'
                  }
                }}
              >
                {mode === 'dark' ? <Brightness7Icon /> : <Brightness4Icon />}
              </IconButton>
            </Tooltip>

            {/* User Info */}
            <Box 
              sx={{ 
                display: 'flex', 
                alignItems: 'center', 
                gap: 1.5,
                cursor: 'pointer',
                px: 1.5,
                py: 0.75,
                borderRadius: 2,
                '&:hover': {
                  backgroundColor: mode === 'dark' ? 'rgba(255,255,255,0.05)' : 'rgba(0,0,0,0.04)'
                }
              }}
              onClick={handleUserMenuOpen}
            >
              <Box sx={{ textAlign: 'right', display: { xs: 'none', sm: 'block' } }}>
                <Typography variant="body2" sx={{ color: textPrimary, fontWeight: 600, lineHeight: 1.2 }}>
                  {currentUser.name}
                </Typography>
                <Typography variant="caption" sx={{ color: textMuted }}>
                  {currentUser.role}
                </Typography>
              </Box>
              <Avatar 
                sx={{ 
                  width: 40, 
                  height: 40, 
                  bgcolor: '#3b82f6',
                  fontSize: '1rem',
                  fontWeight: 600
                }}
              >
                {currentUser.name.split(' ').map(n => n[0]).join('')}
              </Avatar>
            </Box>

            {/* User Menu */}
            <Menu
              anchorEl={anchorEl}
              open={Boolean(anchorEl)}
              onClose={handleUserMenuClose}
              transformOrigin={{ horizontal: 'right', vertical: 'top' }}
              anchorOrigin={{ horizontal: 'right', vertical: 'bottom' }}
              PaperProps={{
                sx: {
                  mt: 1,
                  minWidth: 200,
                  background: bgSecondary,
                  border: `1px solid ${borderColor}`
                }
              }}
            >
              <Box sx={{ px: 2, py: 1.5, borderBottom: `1px solid ${borderColor}` }}>
                <Typography variant="body2" sx={{ color: textPrimary, fontWeight: 600 }}>
                  {currentUser.name}
                </Typography>
                <Typography variant="caption" sx={{ color: textMuted }}>
                  {currentUser.email}
                </Typography>
              </Box>
              <MenuItem onClick={handleUserMenuClose} sx={{ color: textPrimary }}>
                <ListItemIcon sx={{ color: textSecondary }}>
                  <PersonIcon fontSize="small" />
                </ListItemIcon>
                Profile
              </MenuItem>
              <MenuItem onClick={handleUserMenuClose} sx={{ color: textPrimary }}>
                <ListItemIcon sx={{ color: textSecondary }}>
                  <SettingsIcon fontSize="small" />
                </ListItemIcon>
                Settings
              </MenuItem>
              <Divider sx={{ borderColor }} />
              <MenuItem onClick={handleUserMenuClose} sx={{ color: '#f44336' }}>
                <ListItemIcon sx={{ color: '#f44336' }}>
                  <LogoutIcon fontSize="small" />
                </ListItemIcon>
                Logout
              </MenuItem>
            </Menu>
          </Box>
        </Toolbar>
      </AppBar>

      {/* Drawer */}
      <Box
        component="nav"
        sx={{ width: { md: drawerWidth }, flexShrink: { md: 0 } }}
      >
        {/* Mobile drawer */}
        <Drawer
          variant="temporary"
          open={mobileOpen}
          onClose={handleDrawerToggle}
          ModalProps={{ keepMounted: true }}
          sx={{
            display: { xs: 'block', md: 'none' },
            '& .MuiDrawer-paper': {
              boxSizing: 'border-box',
              width: drawerWidth,
              background: bgGradient,
              borderRight: `1px solid ${borderColor}`
            }
          }}
        >
          {drawer}
        </Drawer>
        
        {/* Desktop drawer */}
        <Drawer
          variant="permanent"
          sx={{
            display: { xs: 'none', md: 'block' },
            '& .MuiDrawer-paper': {
              boxSizing: 'border-box',
              width: drawerWidth,
              background: bgGradient,
              borderRight: `1px solid ${borderColor}`
            }
          }}
          open
        >
          {drawer}
        </Drawer>
      </Box>

      {/* Main content */}
      <Box
        component="main"
        sx={{
          flexGrow: 1,
          p: 3,
          width: { md: `calc(100% - ${drawerWidth}px)` },
          minHeight: '100vh',
          background: mode === 'dark' 
            ? 'linear-gradient(135deg, #0f0f1a 0%, #1a1a2e 50%, #16213e 100%)'
            : 'linear-gradient(135deg, #f5f5f5 0%, #fafafa 50%, #f0f0f0 100%)',
          mt: 8
        }}
      >
        {getCurrentComponent()}
      </Box>
    </Box>
  );
};

export default Layout;
