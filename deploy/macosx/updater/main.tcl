

# Sorry for the bad code, but this is my first Tcl script in years...
# and never really understood this weird language -- fg

# TODO: Might fix this script for linux some day
# (it works on linux and even windows, but creates a *.app folder as output, with the mac binaries etc

package require Tcl 8.4
package require Tk

set ::OUTAPP "Aquaria-1.1.3+.app"

# setup GUI
tk appname "Aquaria updater (1.1.x -> 1.1.3+)"

label .status -anchor w -height 2 -font "tkDefaultFont 10" \
    -text "Click Start. The updater will try to find your installation.
No files will be altered without your confirmation."

label .detail -anchor nw -height 3 -font "Helvetica 10"
button .btn -text "Start updating" -command { doUpdate }

image create photo imgSplash -file "splash.gif"
label .splash -image imgSplash

bind  .status <Configure> [list %W configure -wraplength %w]
bind  .detail <Configure> [list %W configure -wraplength %w]

pack .splash -expand y
pack .status -fill x -expand y
pack .detail -fill x -expand y
pack .btn -pady 15

update
wm minsize . [image width imgSplash] [winfo height .]
wm maxsize . [image width imgSplash] [winfo height .]

proc center_window {} {
    wm withdraw .
    update idletasks
    set x [expr [winfo screenwidth .]/2 - [winfo reqwidth .]/2 \
          - [winfo vrootx .]]
    set y [expr [winfo screenheight .]/2 - [winfo reqheight .]/2 \
          - [winfo vrooty .]]
    wm geom . +$x+$y
    wm deiconify .
}

# this is necessary on OSX, otherwise this pops up in the top left corner
center_window
# workaround for quirky window managers (see http://wiki.tcl.tk/1254)
after idle center_window

# end GUI

proc msgboxCompat { type title message detail } {
    if { [catch {
        set ret [tk_messageBox -type $type -title $title -message $message -detail $detail]
    }] } {
        # no -detail on OSX <= 10.5
        set ret [tk_messageBox -type $type -title $title -message "$message\n\n$detail"]
    }
    return $ret
}

proc msgboxYesNo { title message detail } {
    set ret [msgboxCompat yesno $title $message $detail]
    return $ret
}

proc msgboxOk { title message detail } {
    set ret [msgboxCompat ok $title $message $detail]
    return $ret
}

proc setstatus s {
    .status configure -text $s
    update
}

proc setdetail s {
    .detail configure -text $s
    update
}

proc getVendor path {
    if { [isAmbrosia $path] } {
        return "Ambrosia software \[Mac\]"
    }
    if { [file exists "$path/Contents/MacOS/aquaria"]
      && [file exists "$path/Contents/MacOS/libSDL-1.2.0.dylib"]
      && [file isdirectory "$path/ambrosia"]
    } {
        return "HiB \[Mac\]"
    }
    if { [file exists "$path/Aquaria.exe"]
      && [file exists "$path/AQConfig.exe"]
      && [file exists "$path/fmodex.dll"]
    } {
        return "Plimus/HiB \[Win32\]"
    }
    if { [file exists "$path/libSDL-1.2.so.0"] } {
        return "HiB \[Linux\]"
    }
    
    return "Opensource/Unknown"
}

proc useThatDir path {
    set detail "$path
    
(Vendor: [getVendor $path])

Use it to create the updated version? Select \"No\" to continue searching for another one.

The original application will not be changed, and will still be usable afterwards."

    set answer [msgboxYesNo "Question" "Found Aquaria app:" $detail]
    return [string equal $answer "yes"]
}

proc checkSubdirs { path } {
    set need {data gfx mus scripts sfx vox}
    foreach i $need {
        if { ! [file isdirectory [file normalize "$path/$i"]] } {
            return 0
        }
    }
    return 1
}

proc checkExe path {
    if { [file isfile "$path/Contents/MacOS/aquaria"]
      || [file isfile "$path/Contents/MacOS/Aquaria"]
      || [file isfile "$path/Aquaria.exe"]
      || [file isfile "$path/aquaria"]
    } {
        return 1
    }
    return 0
}

proc isAquariaApp path {
    if { [checkExe $path] } {
        if { [checkSubdirs $path] } {
            return 1
        }
        if { [isAmbrosia $path] } {
            if { [checkSubdirs "$path/Contents/Resources"] } {
                return 1
            }
        }
    }
    return 0
}

proc suitable { path } {
    if { [isAquariaApp $path] } {
        return [useThatDir $path]
    }
    return 0
}

proc findInstall { basedir } {
    return [findInstallRec $basedir 0]
}

set ::IGNOREPATHS {"/System" "/Developer" "/Library" "/dev" "/proc" "/net"}
proc checkableEntry name {
    if { [lsearch $::IGNOREPATHS $name] >= 0 } {
        return 0
    }
    if { ! [file exists $name] } {
        return 0
    }
    if { [string equal [file type $name] "link"] } {
        return 0
    }
    # no -hidden on OSX <= 10.5
    catch {
        if { [file attributes $name -hidden] } {
            return 0
        }
    }
    return 1
}

proc filter {list script} {
   set res {}
   foreach e $list {if {[uplevel 1 [list $script $e]]} {lappend res $e}}
   set res
 }

proc findInstallRec { basedir nest } {

    #puts "\[$nest\] $basedir"

    # prevent stack overflow due to bad symlinks
    if { $nest > 15 } {
        return {}
    }
    
    # do not recurse into apps
    if { [string match "*.app*" $basedir] } {
        return {}
    }
    
    # Fix the directory name, this ensures the directory name is in the
    # native format for the platform and contains a final directory seperator
    set basedir [string trimright [file join [file normalize $basedir] { }]]
    
    setdetail $basedir
    set dirs [lsort [filter [glob -nocomplain -type {d  r} -path $basedir *] checkableEntry]]
    
    foreach dn $dirs {
        if { [suitable $dn] } {
            return $dn
        }
    }
    
    foreach dn $dirs {
        set ret [findInstallRec $dn [expr $nest + 1]]
        if { [string length $ret] > 0 } {
            return $ret
        }
    }

    return {}
}

# because [file copy] is wimpy and fails for read-protected mac metadata
proc copyFilesHarder { src dst } {

    set src "[file normalize $src]/"
    set dst "[file normalize $dst]/"

    # just in case
    if { ! [file isdirectory $dst] } {
        file mkdir $dst
    }
    
    set files [lsort [glob -nocomplain -type {f r d} -path $src *]]
    
    # copy files
    foreach f $files {
        set t [file tail $f]
        if { [file isdirectory $f] } {
            copyFilesHarder $f "$dst/$t"
        } else {
            # do not copy file if it already exists in the same location. note that $dst has a trailing / here.
            if { ! [string equal $f "$dst$t"] } {
                setdetail $t
                file copy -force $f $dst
            }
        }
    }
}

proc doUpdate {} {
    .btn configure -state disabled -text "Working..."
    setstatus "Searching for Aquaria application..."
    set app [findInstall "/"]
    
    if { [string length $app] > 0 } {
        set target "[file dirname $app]/$::OUTAPP"
        file mkdir $target
        copyBaseFiles $app $target
        copyUpdateFiles $target
        copyUserFiles
        file attributes "$target/Contents/MacOS/aquaria" -permissions "+x" 
        
        set answer [msgboxYesNo "Question" "Finished! The newly built app is located at:" "$target\n\nDo you want to start it now?"]
        if { [string equal $answer yes] } {
            doLaunch $target
        }
        exit 0
        
        
    } else {
        msgboxOk "Nope." "Nothing found!" \
            "Sorry, but this updater was unable to find a complete Aquaria installation. You can still update manually, though."
        exit 1
    }
}

proc isAmbrosia path {
    # this does not exist in the HiB version
    return [file isdirectory [file normalize "$path/Contents/Frameworks/AmbrosiaTools.framework"]]
}

proc copyBaseFiles { from to } {
    setstatus "Copying files from old application... This may take a while."
    
    # all except scripts
    set fl {data gfx mus sfx vox Contents}
    
    if { [isAmbrosia $from] } {
        foreach f $fl {
            copyFilesHarder "$from/Contents/Resources/$f" "$to/$f"
        }
    } else {
        foreach f $fl {
            copyFilesHarder "$from/$f" "$to/$f"
        }
    }
}

proc copyUpdateFiles { to } {

    setstatus "Copying updated game files..."
    copyFilesHarder "../files/" $to
}

proc copyUserFiles {} {
    set udir "~/Library/Application Support/Aquaria"
        
    setstatus "Copying updated user files..."
    copyFilesHarder "../user/" $udir
}

proc doLaunch app {
    cd $app
    exec [file nativename "Contents/MacOS/aquaria"] &
    exit 0
}

