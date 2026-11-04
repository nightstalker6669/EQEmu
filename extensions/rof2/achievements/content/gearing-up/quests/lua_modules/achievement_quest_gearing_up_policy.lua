-- PRIVATE DISABLED candidate. Unverified eligibility is not unrestricted access.
return {
 enabled=false,
 with_oculus=false, -- Set true only when replacing the pinned, installed Oculus adapter.
 minimum_level=nil,
 admit=function(c)return false end
}
