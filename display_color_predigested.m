function display_color_read
    s = serial('COM3');
    close all
    fopen(s);
    
    finishup = onCleanup(@() cleanup(s));           % removes remaining data from the serial reader                                                                                            
    function cleanup(s)
        fclose(s);                                % close the serial reader
        delete(s);                                % delete all information stored in the serial
        clear s                                   % remove data from MATLAB
        disp('Clean!')                            % tells you it cleaned: if it doesn't unplug and replug arduino
    end
    thing = figure;
    while(1)
        if(get(s, 'BytesAvailable') >= 1)                 % if there is data coming in from the arduino
            idn = sscanf(fscanf(s), '%f%f%f');
            disp(idn);
            thing.Color = [min(max(idn(1), 0), 1), min(max(idn(2), 0), 1), min(max(idn(3), 0), 1)];
        end
        drawnow
    end
end
            