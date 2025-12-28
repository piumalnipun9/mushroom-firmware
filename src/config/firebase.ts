import { initializeApp } from 'firebase/app';
import { getDatabase } from 'firebase/database';

const firebaseConfig = {
  apiKey: "AIzaSyDwwpLAITscXtUaGdD0px0iUOxi3qD3GSo",
  authDomain: "project-mushroom-2f8a9.firebaseapp.com",
  databaseURL: "https://project-mushroom-2f8a9-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "project-mushroom-2f8a9",
  storageBucket: "project-mushroom-2f8a9.firebasestorage.app",
  messagingSenderId: "372300294550",
  appId: "1:372300294550:web:d42110d482305d77ff15b7",
  measurementId: "G-J6B4NVC30J"
};

const app = initializeApp(firebaseConfig);
export const database = getDatabase(app);
export default app;
