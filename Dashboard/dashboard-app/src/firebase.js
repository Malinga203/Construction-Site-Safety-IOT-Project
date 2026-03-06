import { initializeApp } from "firebase/app";
import { getDatabase } from "firebase/database";

const firebaseConfig = {
  apiKey: "AIzaSyAeWvWmLjq_DeCtZPcptmgYt1HM41FtyxI",
  authDomain: "iot-smart-vest.firebaseapp.com",
  databaseURL: "https://iot-smart-vest-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "iot-smart-vest",
  storageBucket: "iot-smart-vest.firebasestorage.app",
  messagingSenderId: "336407535046",
  appId: "1:336407535046:web:0dd6878524ed7856e926c7"
};

const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

export { database };