import { useState, useEffect } from 'react';
import { BOOT_TIME } from '../constants/config';

export function useUptime() {
  const [uptime, setUptime] = useState('00:00:00');

  useEffect(() => {
    const id = setInterval(() => {
      const s   = Math.floor((Date.now() - BOOT_TIME) / 1000);
      const h   = String(Math.floor(s / 3600)).padStart(2, '0');
      const m   = String(Math.floor((s % 3600) / 60)).padStart(2, '0');
      const sec = String(s % 60).padStart(2, '0');
      setUptime(`${h}:${m}:${sec}`);
    }, 1000);
    return () => clearInterval(id);
  }, []);

  return uptime;
}