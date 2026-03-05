import React, { useEffect, useState } from 'react';
import {
    Box,
    Typography,
    Paper,
    List,
    ListItem,
    ListItemText,
    ListItemIcon,
    Chip,
    Divider,
    Button,
    Dialog,
    IconButton
} from '@mui/material';
import WarningIcon from '@mui/icons-material/Warning';
import ErrorIcon from '@mui/icons-material/Error';
import InfoIcon from '@mui/icons-material/Info';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import NotificationsActiveIcon from '@mui/icons-material/NotificationsActive';
import CheckIcon from '@mui/icons-material/Check';
import CloseIcon from '@mui/icons-material/Close';
import BugReportIcon from '@mui/icons-material/BugReport';
import BlockIcon from '@mui/icons-material/Block';
import { Alert as AlertType } from '../../types';
import { subscribeAlerts, acknowledgeAlert, subscribeLatestSystemStatus, markFalseAlarm } from '../../services/firebaseService';

const getAlertIcon = (type: string) => {
    switch (type) {
        case 'error': return <ErrorIcon sx={{ color: '#f44336' }} />;
        case 'warning': return <WarningIcon sx={{ color: '#fbbf24' }} />;
        case 'success': return <CheckCircleIcon sx={{ color: '#4caf50' }} />;
        case 'pest': return <BugReportIcon sx={{ color: '#ec4899' }} />;
        case 'info':
        default: return <InfoIcon sx={{ color: '#3b82f6' }} />;
    }
};

const Alerts: React.FC = () => {
    const [alerts, setAlerts] = useState<AlertType[]>([]);
    const [systemStatus, setSystemStatus] = useState<{ status: string, time: string } | null>(null);
    const [selectedImage, setSelectedImage] = useState<string | null>(null);

    useEffect(() => {
        const unsubscribeAlerts = subscribeAlerts((data) => {
            // Sort so newest is first
            const sortedData = [...data].sort((a, b) => b.timestamp - a.timestamp);
            setAlerts(sortedData);
        });

        const unsubscribeStatus = subscribeLatestSystemStatus((data) => {
            setSystemStatus(data);
        });

        return () => {
            unsubscribeAlerts();
            unsubscribeStatus();
        };
    }, []);

    const handleAcknowledge = async (id: string, e?: React.MouseEvent) => {
        if (e) e.stopPropagation();
        await acknowledgeAlert(id);
    };

    const handleFalseAlarm = async (id: string, e?: React.MouseEvent) => {
        if (e) e.stopPropagation();
        await markFalseAlarm(id);
        await acknowledgeAlert(id);
    };

    const handleAcknowledgeAll = async () => {
        const unacknowledged = alerts.filter(a => !a.acknowledged);
        for (const alert of unacknowledged) {
            if (alert.id) {
                await acknowledgeAlert(alert.id);
            }
        }
    };

    const unacknowledgedCount = alerts.filter(a => !a.acknowledged).length;

    return (
        <Box>
            <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', mb: 3 }}>
                <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                    <NotificationsActiveIcon sx={{ fontSize: 40, color: unacknowledgedCount > 0 ? '#f44336' : '#3b82f6' }} />
                    <Typography
                        variant="h4"
                        color="text.primary"
                        sx={{ fontWeight: 700 }}
                    >
                        System Alerts
                    </Typography>
                </Box>
                <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                    {systemStatus && (
                        <Chip
                            icon={systemStatus.status === 'Healthy' ? <CheckCircleIcon /> : <WarningIcon />}
                            label={`${systemStatus.status} ${systemStatus.time ? `(${new Date(systemStatus.time).toLocaleString()})` : ''}`}
                            color={systemStatus.status === 'Healthy' ? 'success' : 'warning'}
                            sx={{ fontWeight: 'bold', fontSize: '1rem', height: 40, px: 1 }}
                        />
                    )}
                    {unacknowledgedCount > 0 && (
                        <Button
                            variant="outlined"
                            color="primary"
                            startIcon={<CheckIcon />}
                            onClick={handleAcknowledgeAll}
                        >
                            Acknowledge All
                        </Button>
                    )}
                </Box>
            </Box>

            <Paper elevation={2} sx={{ p: 0, borderRadius: 2, overflow: 'hidden' }}>
                {alerts.length === 0 ? (
                    <Box sx={{ p: 4, textAlign: 'center' }}>
                        <Typography color="text.secondary">No alerts found in the system.</Typography>
                    </Box>
                ) : (
                    <List sx={{ width: '100%', bgcolor: 'background.paper', p: 0 }}>
                        {alerts.map((alert, index) => (
                            <React.Fragment key={alert.id || index}>
                                <ListItem
                                    alignItems="flex-start"
                                    sx={{
                                        p: 3,
                                        bgcolor: alert.acknowledged ? 'transparent' : 'action.hover',
                                        transition: 'background-color 0.3s'
                                    }}
                                    secondaryAction={
                                        !alert.acknowledged && (
                                            <Box sx={{ display: 'flex', gap: 1 }}>
                                                {alert.type === 'pest' && !alert.false_alarm && (
                                                    <Button
                                                        variant="outlined"
                                                        size="small"
                                                        color="error"
                                                        onClick={(e) => alert.id && handleFalseAlarm(alert.id, e)}
                                                        sx={{ borderRadius: 2 }}
                                                        startIcon={<BlockIcon />}
                                                    >
                                                        False Alarm
                                                    </Button>
                                                )}
                                                <Button
                                                    variant="contained"
                                                    size="small"
                                                    color="primary"
                                                    onClick={(e) => alert.id && handleAcknowledge(alert.id, e)}
                                                    sx={{ borderRadius: 2 }}
                                                >
                                                    Acknowledge
                                                </Button>
                                            </Box>
                                        )
                                    }
                                >
                                    <ListItemIcon sx={{ mt: 1 }}>
                                        {getAlertIcon(alert.type || 'info')}
                                    </ListItemIcon>
                                    <ListItemText
                                        primary={
                                            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 0.5 }}>
                                                <Typography variant="subtitle1" sx={{ fontWeight: alert.acknowledged ? 500 : 700, color: 'text.primary' }}>
                                                    {alert.type ? alert.type.charAt(0).toUpperCase() + alert.type.slice(1) : 'Info'} Alert
                                                </Typography>
                                                {!alert.acknowledged && (
                                                    <Chip label="New" size="small" color="error" sx={{ height: 20, fontSize: '0.7rem', fontWeight: 'bold' }} />
                                                )}
                                                {alert.false_alarm && (
                                                    <Chip label="False Alarm" size="small" sx={{ height: 20, fontSize: '0.7rem', fontWeight: 'bold', bgcolor: 'action.disabledBackground' }} />
                                                )}
                                            </Box>
                                        }
                                        secondary={
                                            <React.Fragment>
                                                <Typography
                                                    sx={{ display: 'block', mb: 1 }}
                                                    component="span"
                                                    variant="body1"
                                                    color="text.secondary"
                                                >
                                                    {alert.message}
                                                </Typography>
                                                {alert.image && (
                                                    <Box
                                                        sx={{ mb: 1, mt: 1, cursor: 'pointer', maxWidth: 200, borderRadius: 1, overflow: 'hidden', border: '1px solid', borderColor: 'divider' }}
                                                        onClick={(e) => { e.stopPropagation(); setSelectedImage(alert.image!); }}
                                                    >
                                                        <img src={alert.image.startsWith('data:image') ? alert.image : `data:image/jpeg;base64,${alert.image}`} alt="Alert snapshot" style={{ width: '100%', display: 'block' }} />
                                                    </Box>
                                                )}
                                                <Typography
                                                    variant="caption"
                                                    color="text.disabled"
                                                >
                                                    {new Date(alert.timestamp).toLocaleString()}
                                                </Typography>
                                            </React.Fragment>
                                        }
                                    />
                                </ListItem>
                                {index < alerts.length - 1 && <Divider component="li" />}
                            </React.Fragment>
                        ))}
                    </List>
                )}
            </Paper>

            <Dialog
                open={!!selectedImage}
                onClose={() => setSelectedImage(null)}
                maxWidth="lg"
                fullWidth
            >
                <Box sx={{ position: 'relative', bgcolor: 'black', display: 'flex', alignItems: 'center', justifyContent: 'center', minHeight: 400 }}>
                    <IconButton
                        onClick={() => setSelectedImage(null)}
                        sx={{ position: 'absolute', top: 8, right: 8, color: 'white', bgcolor: 'rgba(0,0,0,0.5)', '&:hover': { bgcolor: 'rgba(0,0,0,0.7)' } }}
                    >
                        <CloseIcon />
                    </IconButton>
                    {selectedImage && (
                        <img
                            src={selectedImage.startsWith('data:image') ? selectedImage : `data:image/jpeg;base64,${selectedImage}`}
                            alt="Fullscreen Alert"
                            style={{ maxWidth: '100%', maxHeight: '90vh', objectFit: 'contain' }}
                        />
                    )}
                </Box>
            </Dialog>
        </Box >
    );
};

export default Alerts;
