clear
clc
close all

%% Parámetros

Vin = 0.1;                 % Amplitud de entrada [V pico]

%% Buscar archivos

files = dir('Overdrive_simple*');
files = files(~[files.isdir]);      % Eliminar carpetas

if isempty(files)
    error('No se encontraron archivos Overdrive_simple*');
end

N = length(files);

pot  = [];
Vpp  = [];
Vp   = [];
Vrms = [];
Gain = [];
Gain_dB = [];

%% Procesar archivos

for k = 1:N

    filename = files(k).name;
    fprintf('Procesando: %s\n', filename);

    % Extraer el último número del nombre
    nums = regexp(filename,'\d+','match');

    if isempty(nums)
        fprintf(' -> Ignorado (no contiene número)\n');
        continue
    end

    pot_value = str2double(nums{end});

    %% Leer archivo

    try
        T = readtable(filename);

        t = T{:,1};
        v = T{:,2};

    catch

        data = readmatrix(filename);

        t = data(:,1);
        v = data(:,2);

    end

    % Eliminar NaN
    valid = ~(isnan(t) | isnan(v));

    t = t(valid);
    v = v(valid);

    if isempty(v)
        fprintf(' -> Archivo vacío\n');
        continue
    end

    %% Analizar únicamente el estado estacionario

    idx = t > t(end)/2;

    if any(idx)
        v = v(idx);
    end

    %% Parámetros de salida

    Vpp_out  = max(v)-min(v);
    Vp_out   = max(abs(v));
    Vrms_out = rms(v);

    % Ganancia
    gain = Vp_out/Vin;
    gain_db = 20*log10(gain);

    %% Guardar resultados

    pot(end+1)      = pot_value;
    Vpp(end+1)      = Vpp_out;
    Vp(end+1)       = Vp_out;
    Vrms(end+1)     = Vrms_out;
    Gain(end+1)     = gain;
    Gain_dB(end+1)  = gain_db;

end

%% Ordenar resultados

[pot,idx] = sort(pot);

Vpp     = Vpp(idx);
Vp      = Vp(idx);
Vrms    = Vrms(idx);
Gain    = Gain(idx);
Gain_dB = Gain_dB(idx);

%% Gráficos

figure

subplot(2,2,1)
plot(pot,Gain_dB,'o-','LineWidth',2)
grid on
xlabel('Valor del potenciómetro')
ylabel('Ganancia [dB]')
title('Ganancia')

subplot(2,2,2)
plot(pot,Vpp,'o-','LineWidth',2)
grid on
xlabel('Valor del potenciómetro')
ylabel('V_{pp} [V]')
title('Salida pico a pico')

subplot(2,2,3)
plot(pot,Vrms,'o-','LineWidth',2)
grid on
xlabel('Valor del potenciómetro')
ylabel('V_{RMS} [V]')
title('Salida RMS')

subplot(2,2,4)
plot(pot,Vp,'o-','LineWidth',2)
grid on
xlabel('Valor del potenciómetro')
ylabel('V_{P} [V]')
title('Salida pico')

sgtitle('Caracterización del Overdrive')


%% Mostrar tabla de resultados

Resultados = table( ...
    pot(:), ...
    Vp(:), ...
    Vpp(:), ...
    Vrms(:), ...
    Gain(:), ...
    Gain_dB(:), ...
    'VariableNames', ...
    {'Pot','Vp','Vpp','Vrms','Ganancia','Ganancia_dB'});

disp(Resultados)