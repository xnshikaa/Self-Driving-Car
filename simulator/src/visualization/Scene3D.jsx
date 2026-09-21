import React, { useEffect, useRef } from 'react';
import * as THREE from 'three';

export default function Scene3D({ telemetry, scenarioName, isRunning }) {
  const mountRef = useRef(null);

  const sceneRef = useRef(null);
  const cameraRef = useRef(null);
  const accCarRef = useRef(null);
  const leadCarRef = useRef(null);
  const accWheelsRef = useRef([]);
  const leadWheelsRef = useRef([]);
  const laserRef = useRef(null);
  const coneRef = useRef(null);
  const targetGapMarkerRef = useRef(null);
  const safetyZoneRef = useRef(null);
  const accBrakeLightsRef = useRef([]);
  const leadBrakeLightsRef = useRef([]);

  useEffect(() => {
    const container = mountRef.current;
    if (!container) return;

    const width = container.clientWidth;
    const height = container.clientHeight || 520;

    // 1. Scene
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x070a12);
    scene.fog = new THREE.FogExp2(0x070a12, 0.025);
    sceneRef.current = scene;

    // 2. Camera
    const camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 150);
    camera.position.set(-3.5, 3.2, -4);
    camera.lookAt(0, 0.5, 3);
    cameraRef.current = camera;

    // 3. Renderer
    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(width, height);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.shadowMap.enabled = true;
    renderer.shadowMap.type = THREE.PCFShadowMap;
    container.appendChild(renderer.domElement);

    // 4. Lighting
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.75);
    scene.add(ambientLight);

    const dirLight = new THREE.DirectionalLight(0xffffff, 1.4);
    dirLight.position.set(15, 25, 10);
    dirLight.castShadow = true;
    dirLight.shadow.mapSize.width = 2048;
    dirLight.shadow.mapSize.height = 2048;
    scene.add(dirLight);

    // 5. Track Environment (200 meters long along Z)
    const grid = new THREE.GridHelper(300, 300, 0x334155, 0x1e293b);
    grid.position.set(0, 0, 100);
    scene.add(grid);

    // Asphalt Road Surface
    const roadGeo = new THREE.PlaneGeometry(5.0, 300);
    const roadMat = new THREE.MeshStandardMaterial({ color: 0x0f172a, roughness: 0.85 });
    const road = new THREE.Mesh(roadGeo, roadMat);
    road.rotation.x = -Math.PI / 2;
    road.position.set(0, 0.001, 100);
    road.receiveShadow = true;
    scene.add(road);

    // White Outer Lane Lines
    const lineGeo = new THREE.PlaneGeometry(0.12, 300);
    const lineMat = new THREE.MeshBasicMaterial({ color: 0xf8fafc, side: THREE.DoubleSide });

    const leftLine = new THREE.Mesh(lineGeo, lineMat);
    leftLine.rotation.x = -Math.PI / 2;
    leftLine.position.set(-2.2, 0.01, 100);
    scene.add(leftLine);

    const rightLine = new THREE.Mesh(lineGeo, lineMat);
    rightLine.rotation.x = -Math.PI / 2;
    rightLine.position.set(2.2, 0.01, 100);
    scene.add(rightLine);

    // Center Dashed Line
    for (let z = 0; z < 200; z += 3) {
      const dashGeo = new THREE.PlaneGeometry(0.08, 1.5);
      const dashMat = new THREE.MeshBasicMaterial({ color: 0xe2e8f0, side: THREE.DoubleSide });
      const dash = new THREE.Mesh(dashGeo, dashMat);
      dash.rotation.x = -Math.PI / 2;
      dash.position.set(0, 0.011, z);
      scene.add(dash);
    }

    // --- HERO ACC VEHICLE MODEL (Blue/Teal Body) ---
    const accGroup = new THREE.Group();

    // Chassis
    const accBodyGeo = new THREE.BoxGeometry(0.48, 0.24, 0.80);
    const accBodyMat = new THREE.MeshStandardMaterial({ color: 0x06b6d4, metalness: 0.7, roughness: 0.25 });
    const accBody = new THREE.Mesh(accBodyGeo, accBodyMat);
    accBody.position.y = 0.17;
    accBody.castShadow = true;
    accGroup.add(accBody);

    // Onboard OLED Display Box
    const oledBoxGeo = new THREE.BoxGeometry(0.30, 0.14, 0.35);
    const oledBoxMat = new THREE.MeshStandardMaterial({ color: 0x0284c7, metalness: 0.9, roughness: 0.1 });
    const oledBox = new THREE.Mesh(oledBoxGeo, oledBoxMat);
    oledBox.position.set(0, 0.32, -0.05);
    accGroup.add(oledBox);

    // Front Ultrasonic Transducer Sensor Pod
    const podGeo = new THREE.CylinderGeometry(0.045, 0.045, 0.16, 16);
    const podMat = new THREE.MeshStandardMaterial({ color: 0xf8fafc, metalness: 0.9 });
    const pod = new THREE.Mesh(podGeo, podMat);
    pod.rotation.z = Math.PI / 2;
    pod.position.set(0, 0.17, 0.41);
    accGroup.add(pod);

    // Ultrasonic Beam Emission Cone
    const coneGeo = new THREE.ConeGeometry(0.75, 2.5, 16, 1, true);
    const coneMat = new THREE.MeshBasicMaterial({
      color: 0x06b6d4,
      transparent: true,
      opacity: 0.22,
      side: THREE.DoubleSide,
      wireframe: true
    });
    const cone = new THREE.Mesh(coneGeo, coneMat);
    cone.rotation.x = -Math.PI / 2;
    cone.position.set(0, 0.17, 1.6);
    accGroup.add(cone);
    coneRef.current = cone;

    // ACC Rear Brake Lights
    const accBrakeGeo = new THREE.BoxGeometry(0.10, 0.06, 0.02);
    const accBrakeMatL = new THREE.MeshBasicMaterial({ color: 0x7f1d1d });
    const accBrakeMatR = new THREE.MeshBasicMaterial({ color: 0x7f1d1d });

    const accBrakeL = new THREE.Mesh(accBrakeGeo, accBrakeMatL);
    accBrakeL.position.set(-0.18, 0.22, -0.41);
    accGroup.add(accBrakeL);

    const accBrakeR = new THREE.Mesh(accBrakeGeo, accBrakeMatR);
    accBrakeR.position.set(0.18, 0.22, -0.41);
    accGroup.add(accBrakeR);

    accBrakeLightsRef.current = [accBrakeMatL, accBrakeMatR];

    // Wheels
    const wheelGeo = new THREE.CylinderGeometry(0.10, 0.10, 0.07, 16);
    const wheelMat = new THREE.MeshStandardMaterial({ color: 0x1e293b, roughness: 0.9 });
    const accWheels = [];
    [[-0.26, 0.10, 0.24], [0.26, 0.10, 0.24], [-0.26, 0.10, -0.24], [0.26, 0.10, -0.24]].forEach(pos => {
      const w = new THREE.Mesh(wheelGeo, wheelMat);
      w.rotation.z = Math.PI / 2;
      w.position.set(...pos);
      w.castShadow = true;
      accGroup.add(w);
      accWheels.push(w);
    });
    accWheelsRef.current = accWheels;

    // --- 50 CM TARGET GAP MARKER (Cyan projected ring in front of ACC) ---
    const targetRingGeo = new THREE.RingGeometry(0.48, 0.52, 32);
    const targetRingMat = new THREE.MeshBasicMaterial({ color: 0x10b981, side: THREE.DoubleSide, transparent: true, opacity: 0.85 });
    const targetMarker = new THREE.Mesh(targetRingGeo, targetRingMat);
    targetMarker.rotation.x = -Math.PI / 2;
    targetMarker.position.set(0, 0.02, 0.40 + 0.50); // 50 cm in front of bumper
    accGroup.add(targetMarker);
    targetGapMarkerRef.current = targetMarker;

    // --- 20 CM SAFETY ZONE (Red transparent rectangle under bumper) ---
    const safetyZoneGeo = new THREE.PlaneGeometry(0.85, 0.25);
    const safetyZoneMat = new THREE.MeshBasicMaterial({ color: 0xf43f5e, side: THREE.DoubleSide, transparent: true, opacity: 0.35 });
    const safetyZone = new THREE.Mesh(safetyZoneGeo, safetyZoneMat);
    safetyZone.rotation.x = -Math.PI / 2;
    safetyZone.position.set(0, 0.02, 0.40 + 0.125);
    accGroup.add(safetyZone);
    safetyZoneRef.current = safetyZone;

    accGroup.position.set(0, 0, 0);
    scene.add(accGroup);
    accCarRef.current = accGroup;

    // --- HERO LEAD VEHICLE MODEL (Orange/Red Body) ---
    const leadGroup = new THREE.Group();
    const leadBodyGeo = new THREE.BoxGeometry(0.50, 0.26, 0.84);
    const leadBodyMat = new THREE.MeshStandardMaterial({ color: 0xf43f5e, metalness: 0.4, roughness: 0.3 });
    const leadBody = new THREE.Mesh(leadBodyGeo, leadBodyMat);
    leadBody.position.y = 0.18;
    leadBody.castShadow = true;
    leadGroup.add(leadBody);

    // Lead Rear Brake Lights
    const leadBrakeMatL = new THREE.MeshBasicMaterial({ color: 0x7f1d1d });
    const leadBrakeMatR = new THREE.MeshBasicMaterial({ color: 0x7f1d1d });

    const leadBrakeL = new THREE.Mesh(accBrakeGeo, leadBrakeMatL);
    leadBrakeL.position.set(-0.19, 0.23, -0.43);
    leadGroup.add(leadBrakeL);

    const leadBrakeR = new THREE.Mesh(accBrakeGeo, leadBrakeMatR);
    leadBrakeR.position.set(0.19, 0.23, -0.43);
    leadGroup.add(leadBrakeR);

    leadBrakeLightsRef.current = [leadBrakeMatL, leadBrakeMatR];

    // Lead Wheels
    const leadWheels = [];
    [[-0.27, 0.10, 0.26], [0.27, 0.10, 0.26], [-0.27, 0.10, -0.26], [0.27, 0.10, -0.26]].forEach(pos => {
      const w = new THREE.Mesh(wheelGeo, wheelMat);
      w.rotation.z = Math.PI / 2;
      w.position.set(...pos);
      w.castShadow = true;
      leadGroup.add(w);
      leadWheels.push(w);
    });
    leadWheelsRef.current = leadWheels;

    leadGroup.position.set(0, 0, 1.2);
    scene.add(leadGroup);
    leadCarRef.current = leadGroup;

    // --- LIVE DISTANCE RAY LINE ---
    const laserMat = new THREE.LineDashedMaterial({
      color: 0x10b981,
      dashSize: 0.1,
      gapSize: 0.05,
      linewidth: 3
    });
    const laserGeo = new THREE.BufferGeometry().setFromPoints([
      new THREE.Vector3(0, 0.17, 0.41),
      new THREE.Vector3(0, 0.17, 1.2)
    ]);
    const laser = new THREE.Line(laserGeo, laserMat);
    laser.computeLineDistances();
    scene.add(laser);
    laserRef.current = laser;

    // Animation loop
    let animId;
    const animate = () => {
      animId = requestAnimationFrame(animate);
      renderer.render(scene, camera);
    };
    animate();

    const handleResize = () => {
      if (!container) return;
      const w = container.clientWidth;
      const h = container.clientHeight || 520;
      camera.aspect = w / h;
      camera.updateProjectionMatrix();
      renderer.setSize(w, h);
    };
    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
      cancelAnimationFrame(animId);
      if (container.contains(renderer.domElement)) {
        container.removeChild(renderer.domElement);
      }
    };
  }, []);

  // Synchronize 3D visuals with live telemetry step
  useEffect(() => {
    if (!telemetry || !accCarRef.current || !leadCarRef.current) return;

    const carDist = telemetry.true_distance !== undefined ? telemetry.true_distance : (telemetry.t * telemetry.true_speed_mps);
    const gap = telemetry.true_gap_m !== undefined ? telemetry.true_gap_m : 1.2;
    const leadDist = carDist + gap;

    // 1. Move ACC Car and Lead Car along Z track
    accCarRef.current.position.z = carDist;
    leadCarRef.current.position.z = leadDist;

    // 2. Smooth Chase Camera Follow (position camera behind ACC car)
    if (cameraRef.current) {
      cameraRef.current.position.set(-2.8, 2.6, carDist - 3.5);
      cameraRef.current.lookAt(0, 0.5, carDist + 2.5);
    }

    // 3. Rotate Wheels proportional to velocity
    const accSpeed = telemetry.true_speed_mps || telemetry.speed_mps || 0.0;
    const leadSpeed = telemetry.true_lead_mps || telemetry.lead_mps || 0.0;

    accWheelsRef.current.forEach(w => {
      w.rotation.x += accSpeed * 0.15;
    });
    leadWheelsRef.current.forEach(w => {
      w.rotation.x += leadSpeed * 0.15;
    });

    // 4. Update Distance Ray Line
    if (laserRef.current) {
      const p1 = new THREE.Vector3(0, 0.17, carDist + 0.41);
      const p2 = new THREE.Vector3(0, 0.17, leadDist - 0.42);
      laserRef.current.geometry.setFromPoints([p1, p2]);
      laserRef.current.computeLineDistances();

      if (gap < 0.25) {
        laserRef.current.material.color.setHex(0xf43f5e);
      } else if (gap < 0.50) {
        laserRef.current.material.color.setHex(0xf59e0b);
      } else {
        laserRef.current.material.color.setHex(0x10b981);
      }
    }

    // 5. ACC Brake Lights
    const isAccBraking = telemetry.braking === 1 || telemetry.mode === 'EMERG' || (telemetry.target_mps < telemetry.speed_mps - 0.05);
    accBrakeLightsRef.current.forEach(mat => {
      mat.color.setHex(isAccBraking ? 0xff0000 : 0x7f1d1d);
    });

    // 6. Lead Brake Lights
    const isLeadBraking = leadSpeed < 0.08 || (telemetry.mode === 'STOP');
    leadBrakeLightsRef.current.forEach(mat => {
      mat.color.setHex(isLeadBraking ? 0xff0000 : 0x7f1d1d);
    });

    // 7. Safety Zone Pulse
    if (safetyZoneRef.current) {
      safetyZoneRef.current.material.opacity = gap < 0.25 ? 0.75 : 0.30;
      safetyZoneRef.current.material.color.setHex(gap < 0.25 ? 0xf43f5e : (gap < 0.50 ? 0xf59e0b : 0x10b981));
    }
  }, [telemetry]);

  const simTime = telemetry ? telemetry.t : 0.0;

  return (
    <div style={{ position: 'relative', width: '100%', height: '520px', borderRadius: '12px', overflow: 'hidden', border: '2px solid #06b6d4', boxShadow: '0 12px 32px rgba(6,182,212,0.15)' }}>
      <div ref={mountRef} style={{ width: '100%', height: '100%' }} />

      {/* PROMINENT SIM TIME CLOCK (TOP CENTER) */}
      <div style={{
        position: 'absolute',
        top: '14px',
        left: '50%',
        transform: 'translateX(-50%)',
        background: 'rgba(15, 23, 42, 0.92)',
        backdropFilter: 'blur(10px)',
        border: '1px solid #06b6d4',
        padding: '6px 20px',
        borderRadius: '30px',
        color: '#06b6d4',
        fontFamily: 'JetBrains Mono, monospace',
        fontSize: '1.2rem',
        fontWeight: 800,
        letterSpacing: '0.05em',
        boxShadow: '0 4px 20px rgba(6,182,212,0.3)'
      }}>
        SIM TIME: {simTime.toFixed(2)} s
      </div>

      {/* Track Legend Overlay (Bottom-Right) */}
      <div style={{
        position: 'absolute',
        bottom: '14px',
        right: '14px',
        background: 'rgba(15, 23, 42, 0.88)',
        backdropFilter: 'blur(8px)',
        border: '1px solid rgba(255, 255, 255, 0.12)',
        padding: '8px 14px',
        borderRadius: '8px',
        fontSize: '0.75rem',
        fontFamily: 'JetBrains Mono, monospace',
        display: 'flex',
        flexDirection: 'column',
        gap: '4px'
      }}>
        <div style={{ color: '#06b6d4' }}>● Blue Car = Our ACC Vehicle</div>
        <div style={{ color: '#f43f5e' }}>● Red Car = Lead Vehicle</div>
        <div style={{ color: '#10b981' }}>-- Green Ring = Target 50 cm Gap</div>
        <div style={{ color: '#f43f5e' }}>-- Red Box = Safety 20-25 cm Zone</div>
      </div>
    </div>
  );
}
