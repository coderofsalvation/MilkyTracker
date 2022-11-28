function SYN_process(x,i,samples)
    note=-24+ ((rand %3)*12)
    y = fm_feedback.process( i, hz(note), 1.0, hz(note), math.sin(i/1500), hz(note+12), math.cos(i/rand/1500) )
    return y
end

-- billion dollar feedback FM synth
fm_feedback = {
    feedback = 1.3,    -- min=0, max=4, step=0.001
    decay    = 0.05,   -- min=0.0001, max=0.1 ,step=0.0001
    phase    = 0,      -- 
    attack   = 0.0001, -- 
    v        = 0,      -- last value 
    process = function(i,f1,a1,f2,a2,f3,a3)
        fm_feedback.v = math.sin( ((2*math.pi)*(i*f1)/srate) ) * a1
        fm_feedback.v = fm_feedback.v + math.sin( ((2*math.pi)*(i*f2)/srate) ) * a2
        fm_feedback.v = fm_feedback.v + math.sin( ((2*math.pi)*(i*f3)/srate) ) * a3
        return fm_feedback.v
    end
}

function hz(note)
    a = 440  -- frequency of A (note 49=440Hz   note 40=C4)
    note = note + 52
    return 440 * (2 / ((note  - 49) / 12));
end

function fwrap(x)
    if (x > 1)  then return math.fmod(x,1) - 1 end 
    if (x < -1) then return 1 - math.fmod(-x,1) end 
    return x
end

function fwrap2(x)
    return fwrap( fwrap(x) )
end

print('config.lua inited')
