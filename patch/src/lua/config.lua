-- don't edit here, create a user.lua instead (loaded automatically)
version = 0.1

synths = {

  bass = {
    get = function(i,note)
        if note == nil then note=-24 end
        up    = (rand % 2) * excite.get( math.sin( ((2*math.pi)*(i*hz(note+12))/srate) ), 0.7 )
        return up + math.sin( ((2*math.pi)*(i*hz(note))/srate) )
    end,
    samples    = 88,
    loop_type  = 1,
    loop_start = 0,
    loop_stop  = 88,
  },

  perc = {
    get = function(i,note)
        if note == nil then note=-24+(rand%6)*-12 end
        x = math.sin( ((2*math.pi)*(i*hz(note))/srate) )
        if( i < 50 ) then return 0 end
        return mirror( fadeout(x,i,synths.perc.samples,srate) * 1.1)
    end,
    samples    = 3000,
    loop_type  = 0,
    loop_start = 2000,
    loop_stop  = 2218
  },

  bell = {
    get = function(i,note,a1,f2,a2,f3,a3)
        if note == nil then note=-12 end
        if a1 == nil then 
          f1 = hz(note)
          a1 = 1
          f2 = hz(note+(rand%4)*12) 
          a2 = math.sin(i*0.5)*0.7
          f3 = hz(note+(rand%4)*12)
          a3 = math.cos(i*0.5)
        end
        v = math.sin( ((2*math.pi)*(i*f1)/srate) ) * a1
        v = v + math.sin( ((2*math.pi)*(i*f2)/srate) ) * a2
        v = v + math.sin( ((2*math.pi)*(i*f3)/srate) ) * a3
        return excite.get( clamp( v * 2 ), 0.5 )
    end,
    samples    = 5000,
    loop_type  = 2,
    loop_start = 27,
    loop_stop  = 4990,
  },

  pwm = {
    get = function(i,note,w,var)
      if note == nil then 
        note = hz(-12)
        p = i*(1/srate)*note
        w = math.sin( ((2*math.pi)*(i*hz(note*12))/srate) ) * 0.5
        if( (rand %2) == 0 ) then w = saw( (1/srate) * i ) end
        var = rand%2
      end
      if (var == 0 ) then return saw(p) + -saw(p+w)       end
      if (var == 1 ) then return sawsin(p) + -sawsin(p+w) end
    end, 
    samples    = 44100,
    loop_type  = 2,
    loop_start = 0,
    loop_stop  = 44070
  },

  sinmirror = {
    get = function(i,note)
      if note == nil then note=3*(-12) end
      intensity = 1+((1/5000)*i)
      return mirror( math.sin( ((2*math.pi)*(i*hz(note))/srate) ) * intensity )
    end,
    samples    = 32000,
    loop_type  = 2,
    loop_start = 36,
    loop_stop  = 31940
  }

}

-- sample-editor: rightclick > fx > filter
dialog_filter = function(x,i,samples,srate)
  local lfo = math.abs( math.sin( math.ceil(sliders[4].val/2) * (1*math.pi)*(i/samples) ) )
  local y = hpf.get(x, sliders[1].val, sliders[3].val/100 )
  y = lpf.get(y, sliders[2].val, sliders[3].val/100 )
  if( sliders[4].val > 0 ) then y = ((1-lfo)*x) + lfo*y end
  return y*(sliders[5].val/100)
end

-- sample-editor: rightclick > fx > distort
dialog_distort = function(x,i,samples,srate)
  local lfo = math.abs( math.sin( math.ceil(sliders[5].val/2) * (1*math.pi)*(i/samples) ) )
  local a   = x*(1+(sliders[1].val/10))          -- input
  local b   = a 
  if( sliders[2].val>1 ) then 
    b = fwrap2(  a + (a*(sliders[2].val/2000) ) ) -- harsh
  end
  local c   = b
  if( sliders[3].val>1 ) then
    c   = mirror( b + (b*(sliders[3].val/100) ) ) -- smooth
  end
  local d  = c                                    -- grit
  if( sliders[4].val>1 ) then 
    local f = math.ceil
    if x < 0 then f = math.floor end
    local s = 101-sliders[4].val
    d = f( d * s ) / s
  end
  local e  = d
  if( sliders[5].val > 0 ) then                   -- apply lfo
    e  = ((1-lfo)*x) + (lfo*e)                
  end
  local f = e*(sliders[6].val/100)
  return f
end

-- SYN_process (syn-button in sample-editor)
button_SYN = function(x,i,samples,srate)
    return synth.get(i)
end

-- some effects need to be reset once in a while
reset = function()
  -- reset filters
  lpf.s0=0
  lpf.s1=0
  lpf.a0=0
  lpf.q=0.3 -- 0:1
  hpf.s0=0
  hpf.s1=0
  hpf.a0=0
  hpf.q=0.3 -- 0:1
end

--
-- UTILITIES
--

-- hz(note) converts notenumber to hz
function hz(note)
    local a = 440  -- frequency of A (note 49=440Hz   note 40=C4)
    local n = note + 52
    return 440 * (2 / ((n  - 49) / 12));
end

function fwrap(x)
  if x > 1 then return x % 1  end
  if x < 0 then return x % -1 end
  return x
end

function mirror(x)
  if (x > -1 and x < 1 ) then return x end
  if (x > 1 ) then 
    return 1 + 1 - x 
  else 
    return -1 + -1 - x 
  end
end

function clamp(y)
  if( y >  1 ) then y =  1.0 end
  if( y < -1 ) then y = -1.0 end
  return y
end


function fwrap2(x)
    return fwrap( fwrap(x) )
end

lpf = {
  s0=0,
  s1=0,
  a0=0,
  q=0.3, -- 0:1
  get = function(x,cutoff,q)
    if( q ) then lpf.q = q end
    cutoff = 2.0*math.sin(math.pi*cutoff/srate)
    fb = lpf.q + lpf.q/(1.0 - cutoff )
    hp = x - lpf.s0
    bp = lpf.s0 - lpf.s1
    lpf.s0 = lpf.s0 + cutoff * (hp + fb * bp)
    lpf.s1 = lpf.s1 + cutoff * (lpf.s0 - lpf.s1)
    return lpf.s1
  end
}

hpf = {
  s0=0,
  s1=0,
  a0=0,
  q=0.3, -- 0:1
  get = function(x,cutoff,q)
    if( q ) then hpf.q = q end
    cutoff = 2.0*math.sin(math.pi*cutoff/srate)
    fb = hpf.q + hpf.q/(1.0 - cutoff )
    hp = x - hpf.s0
    bp = hpf.s0 - hpf.s1
    hpf.s0 = hpf.s0 + cutoff * (hp + fb * bp)
    hpf.s1 = hpf.s1 + cutoff * (hpf.s0 - hpf.s1)
    return hpf.s1 - x
  end
}

function saw(phase)
  local f = (phase + 1) % 2
  return f - 1
end

function sawsin(phase)
  local f = (phase ) % 2
  if (f >  0.5) then return math.sin(phase) end
  if (f <= 0.5) then return fwrap( math.sin(phase) * (f+0.5) ) end
end

fadeout = function(x,i,samples,srate)
  return x * (1.0 - ((1.0/samples)*i ) )
end

excite = {
  s0=0,
  get= function(x,amp)
    local y = x + (x-excite.s0)*amp
    excite.s0 = y
    return clamp(y)
  end
}

count = function(T)
  local count = 0
  for _ in pairs(T) do count = count + 1 end
  return count
end

randomsynth = function()
  i = 1
  j = rand % (count(synths))
  for k,s in pairs(synths) do
    synth = s
    if( i == j+1 ) then break end
    i = i+1
  end
  return synth
end

-- assign random synth
synth = randomsynth()

-- load user.lua script (to override functionality)
local success, syntaxError = loadfile("user.lua")
if not success then 
  if( not syntaxError:match("cannot open") ) then
    print("user.lua: syntax error:", syntaxError) 
  end
end
