import React, { useEffect, useState } from "react";
import { ref, onValue } from "firebase/database";
import { database } from "./firebase";

function App() {
  const [data, setData] = useState({
    gasLevel: 0,
    airQuality: 0,
    temperature: 0,
    humidity: 0,
    fall: false,
    gasStatus: "Loading...",
    airStatus: "Loading...",
    lastUpdate: 0
  });

  useEffect(() => {
    const smartVestRef = ref(database, "smart_vest");

    const unsubscribe = onValue(smartVestRef, (snapshot) => {
      if (snapshot.exists()) {
        setData(snapshot.val());
      }
    });

    return () => unsubscribe();
  }, []);

  const getFallBadge = () => {
    return data.fall ? "bg-danger" : "bg-success";
  };

  const getGasBadge = () => {
    if (data.gasLevel < 1000) return "bg-success";
    if (data.gasLevel < 1500) return "bg-warning text-dark";
    if (data.gasLevel < 2500) return "bg-orange text-white";
    return "bg-danger";
  };

  const getAirBadge = () => {
    if (data.airQuality < 1000) return "bg-success";
    if (data.airQuality < 2000) return "bg-warning text-dark";
    if (data.airQuality < 3000) return "bg-orange text-white";
    return "bg-danger";
  };

  return (
    <div className="app-bg min-vh-100">
      <div className="container py-4">
        <div className="text-center mb-4">
          <h1 className="fw-bold dashboard-title">Smart Vest Safety Dashboard</h1>
          <p className="text-light opacity-75 mb-0">
            Real-time monitoring with Firebase
          </p>
        </div>

        <div className="row g-4">
          <div className="col-md-6 col-lg-3">
            <div className="card dashboard-card h-100">
              <div className="card-body text-center">
                <div className="icon-circle bg-danger-subtle">🌡️</div>
                <h5 className="mt-3">Temperature</h5>
                <h2>{Number(data.temperature).toFixed(1)} °C</h2>
              </div>
            </div>
          </div>

          <div className="col-md-6 col-lg-3">
            <div className="card dashboard-card h-100">
              <div className="card-body text-center">
                <div className="icon-circle bg-primary-subtle">💧</div>
                <h5 className="mt-3">Humidity</h5>
                <h2>{Number(data.humidity).toFixed(1)} %</h2>
              </div>
            </div>
          </div>

          <div className="col-md-6 col-lg-3">
            <div className="card dashboard-card h-100">
              <div className="card-body text-center">
                <div className="icon-circle bg-warning-subtle">🔥</div>
                <h5 className="mt-3">Gas Level</h5>
                <h2>{data.gasLevel}</h2>
              </div>
            </div>
          </div>

          <div className="col-md-6 col-lg-3">
            <div className="card dashboard-card h-100">
              <div className="card-body text-center">
                <div className="icon-circle bg-info-subtle">🌫️</div>
                <h5 className="mt-3">Air Quality</h5>
                <h2>{data.airQuality}</h2>
              </div>
            </div>
          </div>
        </div>

        <div className="row g-4 mt-1">
          <div className="col-lg-4">
            <div className="card dashboard-card h-100">
              <div className="card-body">
                <h4 className="mb-3">Gas Status</h4>
                <span className={`badge fs-6 px-3 py-2 ${getGasBadge()}`}>
                  {data.gasStatus}
                </span>
                <p className="mt-3 mb-0 text-muted-light">
                  MQ2 sensor live reading from Firebase.
                </p>
              </div>
            </div>
          </div>

          <div className="col-lg-4">
            <div className="card dashboard-card h-100">
              <div className="card-body">
                <h4 className="mb-3">Air Status</h4>
                <span className={`badge fs-6 px-3 py-2 ${getAirBadge()}`}>
                  {data.airStatus}
                </span>
                <p className="mt-3 mb-0 text-muted-light">
                  MQ135 air quality status in real time.
                </p>
              </div>
            </div>
          </div>

          <div className="col-lg-4">
            <div className="card dashboard-card h-100">
              <div className="card-body">
                <h4 className="mb-3">Fall Detection</h4>
                <span className={`badge fs-6 px-3 py-2 ${getFallBadge()}`}>
                  {data.fall ? "Fall Detected" : "Normal"}
                </span>
                <p className="mt-3 mb-0 text-muted-light">
                  MPU6050-based worker safety monitoring.
                </p>
              </div>
            </div>
          </div>
        </div>

        <div className="card dashboard-card mt-4">
          <div className="card-body">
            <h4 className="mb-3">Live Summary</h4>
            <div className="row">
              <div className="col-md-6">
                <p><strong>Temperature:</strong> {Number(data.temperature).toFixed(1)} °C</p>
                <p><strong>Humidity:</strong> {Number(data.humidity).toFixed(1)} %</p>
                <p><strong>Gas Level:</strong> {data.gasLevel}</p>
                <p><strong>Air Quality:</strong> {data.airQuality}</p>
              </div>
              <div className="col-md-6">
                <p><strong>Gas Status:</strong> {data.gasStatus}</p>
                <p><strong>Air Status:</strong> {data.airStatus}</p>
                <p><strong>Fall Status:</strong> {data.fall ? "YES" : "NO"}</p>
                <p><strong>Last Update:</strong> {data.lastUpdate ? `${data.lastUpdate} ms` : "N/A"}</p>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;