clear
clc
%close all

%% Parámetros

archivo = 'Out_2080';     % Archivo exportado por LTspice
Vin = 0.1;                            % Amplitud de entrada [Vp]

%% Leer archivo completo

fid = fopen(archivo,'r');

if fid==-1
    error('No se pudo abrir el archivo.')
end

% Saltar encabezado
fgetl(fid);

Rgain = [];
Vp = [];
Vpp = [];
Vrms = [];
Gain = [];
Gain_dB = [];

t = [];
v = [];

while ~feof(fid)

    linea = strtrim(fgetl(fid));

    if isempty(linea)
        continue
    end

    %----------------------------------------------------------
    % Nuevo STEP
    %----------------------------------------------------------

    if startsWith(linea,'Step Information')

        % Procesar el bloque anterior
        if ~isempty(v)

            idx = t > t(end)/2;

            if any(idx)
                vv = v(idx);
            else
                vv = v;
            end

            vp = max(abs(vv));
            vpp = max(vv)-min(vv);
            vrms = rms(vv);

            gain = vp/Vin;
            gain_db = 20*log10(gain);

            Vp(end+1)=vp;
            Vpp(end+1)=vpp;
            Vrms(end+1)=vrms;
            Gain(end+1)=gain;
            Gain_dB(end+1)=gain_db;

        end

        % Extraer Rgain
        token = regexp(linea,'Rgain=([0-9\.]+)(K?)','tokens');

        valor = str2double(token{1}{1});

        if strcmp(token{1}{2},'K')
            valor = valor*1000;
        end

        Rgain(end+1)=valor;

        % Reiniciar vectores
        t=[];
        v=[];

        continue
    end

    %----------------------------------------------------------
    % Leer datos
    %----------------------------------------------------------

    datos = sscanf(linea,'%f %f');

    if numel(datos)==2
        t(end+1)=datos(1);
        v(end+1)=datos(2);
    end

end

%% Procesar el último STEP

idx = t > t(end)/2;

if any(idx)
    vv = v(idx);
else
    vv = v;
end

vp = max(abs(vv));
vpp = max(vv)-min(vv);
vrms = rms(vv);

gain = vp/Vin;
gain_db = 20*log10(gain);

Vp(end+1)=vp;
Vpp(end+1)=vpp;
Vrms(end+1)=vrms;
Gain(end+1)=gain;
Gain_dB(end+1)=gain_db;

%% Ordenar

[Rgain,idx]=sort(Rgain);

Vp=Vp(idx);
Vpp=Vpp(idx);
Vrms=Vrms(idx);
Gain=Gain(idx);
Gain_dB=Gain_dB(idx);

%% Tabla

Resultados = table(Rgain(:),Vp(:),Vpp(:),Vrms(:),Gain(:),Gain_dB(:),...
    'VariableNames',{'Rgain_Ohm','Vp','Vpp','Vrms','Ganancia','Ganancia_dB'});

disp(Resultados)

%% Gráficos

figure('Color','w','Position',[100 100 1200 700])

subplot(2,2,1)
plot(Rgain/1000,Gain_dB,'LineWidth',2)
grid on
xlabel('R_{gain} [k\Omega]')
ylabel('Ganancia [dB]')
title('Ganancia del Overdrive')

subplot(2,2,2)
plot(Rgain/1000,Vpp,'LineWidth',2)
grid on
xlabel('R_{gain} [k\Omega]')
ylabel('V_{pp} [V]')
title('Amplitud Pico-Pico')

subplot(2,2,3)
plot(Rgain/1000,Vrms,'LineWidth',2)
grid on
xlabel('R_{gain} [k\Omega]')
ylabel('V_{RMS} [V]')
title('Valor RMS')

subplot(2,2,4)
plot(Rgain/1000,Vp,'LineWidth',2)
grid on
xlabel('R_{gain} [k\Omega]')
ylabel('V_{P} [V]')
title('Amplitud Pico')