function SYN_process(x,i,samples)
    note=-24 --+ ((rand %3)*12)
    y = clamp( fm_feedback.process( i, hz(note), 1.0, hz(note), math.sin(i/1700)*0.7, hz(note+12), math.cos(i/rand/1700) ) * 2)
    return excite.process(y)
end

-- billion dollar feedback FM synth
fm_feedback = {
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
  b = x
  if( x < 0 ) then b = -x  end
  z,c = math.modf(b,1)
  if (x > 1)  then return clamp(x) - c end
  if (x < -1) then return clamp(x) + c end
  return x
end

function fwrap2(x)
    return fwrap( fwrap(x) )
end

lpf = {
  s0=0,
  s1=0,
  a0=0,
  q=0.3, -- 0:1
  process = function(x,cutoff,q)
    if not q == nil then lpf.q = q end
    cutoff = 2.0*math.sin(math.pi*cutoff/srate)
    fb = lpf.q + lpf.q/(1.0 - cutoff )
    hp = x - lpf.s0
    bp = lpf.s0 - lpf.s1
    lpf.s0 = lpf.s0 + cutoff * (hp + fb * bp)
    lpf.s1 = lpf.s1 + cutoff * (lpf.s0 - lpf.s1)
    return lpf.s1
  end
}

function clamp(y)
  if( y >  1 ) then y =  1.0 end
  if( y < -1 ) then y = -1.0 end
  return y
end

hpf = {
  process = function(x,cutoff,q)
    return lpf.process(x,cutoff,q) - x
  end
}

excite = {
  s0=0,
  process= function(x)
    y = fwrap(x + (x - excite.s0*1.1))
    excite.s0 = y
    return y
  end
}



print('config.lua initedd')
print( fwrap(1.2) )
print( fwrap(-1.2) )
