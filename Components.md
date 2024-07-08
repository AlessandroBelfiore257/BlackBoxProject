_Inventario dei sensori utili e stima delle componenti che mi serviranno_

## Componenti e sensori necessari:
- Led utile per "segnalazione allarmi":
3 led (verde, giallo, rosso)
3 resistori 
3 collegamenti grd
3 pin digitali
- Sensore di rilevamento gas e fumo utile per "segnalazione allarmi": 
1 modulo MQ-2
1 collegamento a grd
1 collegamento a Vcc (5V)
1 pin digitale
1 pin analogico
- Accelerometro utile per "segnalazione allarmi/profilazione guida":
1 scheda GY-521 MPU-6050 
1 collegamento a grd
1 collegamento a Vcc (5V)
2 pin analogici (SDA/SCL)
- GPS utile per "tracking/segnalazione allarmi":
1 modulo NEO-6M GPS
1 collegamento a grd
1 collegamento a Vcc (5V)
2 pin digitali (TX/RX)
- Sensore di rilevamento alcool utile per "segnalazione allarmi":
1 modulo MQ-3
1 collegamento a grd
1 collegamento a Vcc (5V)
1 pin analogico
1 pin digitale
- Display utile per "segnalazione allarmi":
1 Display LCD 16X2
1 collegamento a grd
1 collegamento a Vcc (5V)
2 pin analogici (SDA/SCL)
- Pulsante utile per "segnalazione allarmi":
1 pulsante
1 resistore
1 collegamento a grd
1 collegamento a Vcc (5V)
1 pin digitale
- Modulo RTC utile per monitoraggio orario in assenza di connessione
1 collegamento a grd
1 collegamento a Vcc (5V)
2 pin analogici (SDA/SCL)

- Totale collegamenti a grd: 10
- Totale collegamenti a Vcc: 7
- Totale pin digitali sfruttati: 8
- Totale pin analogici sfruttati: 8
- Breadboard da 830 pin: 2
- Stima jumper totali: 40/45
- Stima numero totale di sensori: 8
#### Board proposta: ESP32
##### caratteristiche:
bluetooth 4.2 e ble, modulo wi-fi, dual core, ADC 12 bit, funziona a 3.3V
